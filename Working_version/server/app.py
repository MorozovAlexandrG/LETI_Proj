import logging
import math
import os
import re
from datetime import datetime
from pathlib import Path
from urllib.parse import urlparse, urlunparse

import psycopg
from fastapi import FastAPI, File, Form, Header, HTTPException, Query, Request, UploadFile
from fastapi.responses import FileResponse, RedirectResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from markupsafe import Markup
from psycopg import sql
from psycopg.rows import dict_row

BASE = Path(__file__).resolve().parent
DATA = BASE / "data"
OVERVIEW_POINTS = 2000
WINDOW_SEC = 10.0
NAME_RE = re.compile(
    r"^(?P<patient>.+)_(?P<date>\d{4}-\d{2}-\d{2})_(?P<time>\d{6})(?:-\d+)?\.csv$"
)

logging.basicConfig(level=logging.INFO, format="%(levelname)s %(message)s")
log = logging.getLogger("ecg")

app = FastAPI(title="Холтер")
app.mount("/static", StaticFiles(directory=BASE / "static"), name="static")
templates = Jinja2Templates(directory=BASE / "templates")

db_ready = False
db_error = ""


def database_url() -> str:
    return os.environ.get("DATABASE_URL", "postgresql:///ecg")


def device_token() -> str:
    return os.environ.get("DEVICE_TOKEN", "change-me")


def fmt_size(n: int) -> str:
    if n < 1024:
        return f"{n} Б"
    if n < 1024 * 1024:
        return f"{n / 1024:.1f} КБ"
    return f"{n / (1024 * 1024):.1f} МБ"


def fmt_duration(seconds: float) -> str:
    total = max(0, int(round(seconds)))
    minutes, secs = divmod(total, 60)
    hours, minutes = divmod(minutes, 60)
    if hours:
        return f"{hours}:{minutes:02d}:{secs:02d}"
    return f"{minutes}:{secs:02d}"


def fmt_when(value: datetime) -> str:
    return value.astimezone().strftime("%Y-%m-%d %H:%M:%S")


def tojson(value) -> Markup:
    import json

    return Markup(json.dumps(value, ensure_ascii=False))


templates.env.filters["size"] = fmt_size
templates.env.filters["duration"] = fmt_duration
templates.env.filters["when"] = fmt_when
templates.env.filters["tojson"] = tojson


def safe_token(value: str, fallback: str) -> str:
    cleaned = re.sub(r"[^\w\-]+", "_", value.strip(), flags=re.UNICODE)
    cleaned = cleaned.strip("_")
    return (cleaned or fallback)[:80]


def create_schema(conn) -> None:
    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS recordings (
            id bigserial PRIMARY KEY,
            patient_name text NOT NULL,
            device_id text NOT NULL,
            sample_rate integer NOT NULL,
            received_at timestamptz NOT NULL DEFAULT now(),
            filename text NOT NULL UNIQUE,
            size_bytes bigint NOT NULL,
            duration_sec double precision NOT NULL
        )
        """
    )


def init_db() -> None:
    global db_ready, db_error
    url = database_url()
    parsed = urlparse(url)
    dbname = parsed.path.lstrip("/") or "ecg"
    try:
        with psycopg.connect(url, autocommit=True) as conn:
            create_schema(conn)
        db_ready = True
        db_error = ""
        return
    except psycopg.OperationalError as exc:
        message = str(exc)
        if "does not exist" not in message or "database" not in message:
            db_ready = False
            db_error = message
            return

    admin = urlunparse(parsed._replace(path="/postgres"))
    try:
        with psycopg.connect(admin, autocommit=True) as conn:
            conn.execute(sql.SQL("CREATE DATABASE {}").format(sql.Identifier(dbname)))
        with psycopg.connect(url, autocommit=True) as conn:
            create_schema(conn)
        db_ready = True
        db_error = ""
    except psycopg.Error as exc:
        db_ready = False
        db_error = str(exc)


def ensure_db() -> None:
    if not db_ready:
        init_db()
    if not db_ready:
        raise psycopg.OperationalError(db_error or "PostgreSQL недоступен")


def connect():
    ensure_db()
    return psycopg.connect(database_url(), row_factory=dict_row)


def allocate_name(patient: str, moment: datetime) -> Path:
    stamp = moment.strftime("%Y-%m-%d_%H%M%S")
    candidate = DATA / f"{patient}_{stamp}.csv"
    suffix = 2
    while candidate.exists():
        candidate = DATA / f"{patient}_{stamp}-{suffix}.csv"
        suffix += 1
    return candidate


def data_rows_from_newlines(path: Path, newlines: int) -> int:
    if newlines <= 0 or path.stat().st_size == 0:
        return 0
    rows = newlines - 1
    with path.open("rb") as handle:
        handle.seek(-1, os.SEEK_END)
        if handle.read(1) != b"\n":
            rows += 1
    return max(0, rows)


def recording_or_404(recording_id: int) -> dict:
    with connect() as conn:
        row = conn.execute(
            "SELECT * FROM recordings WHERE id = %s", (recording_id,)
        ).fetchone()
    if row is None:
        raise HTTPException(404, "Запись не найдена")
    return row


def csv_path(filename: str) -> Path:
    if "/" in filename or "\\" in filename or filename.startswith("."):
        raise HTTPException(400, "Неверное имя файла")
    path = (DATA / filename).resolve()
    if not path.is_relative_to(DATA.resolve()) or path.suffix.lower() != ".csv":
        raise HTTPException(400, "Неверное имя файла")
    if not path.is_file():
        raise HTTPException(404, "Файл не найден")
    return path


def read_samples(path: Path, start_index: int, max_points: int, step: int) -> dict:
    times = []
    values = []
    index = 0
    step = max(1, step)
    with path.open("r", encoding="utf-8", errors="replace") as handle:
        handle.readline()
        for line in handle:
            take = index >= start_index and (index - start_index) % step == 0
            index += 1
            if not take:
                continue
            parts = line.strip().split(",")
            if len(parts) < 2:
                continue
            try:
                moment = float(parts[0]) / 1000.0
                value = float(parts[1])
            except ValueError:
                continue
            times.append(round(moment, 3))
            values.append(value)
            if len(times) >= max_points:
                break
    return {"t": times, "ecg": values}


def overview_step(duration_sec: float, sample_rate: int) -> int:
    total = max(0, int(round(duration_sec * sample_rate)))
    if total <= OVERVIEW_POINTS:
        return 1
    return max(1, math.ceil(total / OVERVIEW_POINTS))


def clamp_start(start_sec: float, duration_sec: float, window: float) -> float:
    if duration_sec <= window:
        return 0.0
    return min(max(0.0, start_sec), duration_sec - window)


def patient_from_filename(filename: str) -> str:
    match = NAME_RE.match(filename)
    if match:
        return match.group("patient")
    return Path(filename).stem


def count_data_rows(path: Path) -> int:
    newlines = 0
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            newlines += chunk.count(b"\n")
    return data_rows_from_newlines(path, newlines)


def render_view(request: Request, rec: dict, start_sec: float, page_url: str, download_url: str):
    path = csv_path(rec["filename"])
    duration = float(rec["duration_sec"])
    rate = int(rec["sample_rate"])
    start = clamp_start(start_sec, duration, WINDOW_SEC)
    series = read_samples(path, int(start * rate), max(1, int(WINDOW_SEC * rate)), 1)
    overview = read_samples(path, 0, OVERVIEW_POINTS, overview_step(duration, rate))
    at_start = start <= 0
    at_end = duration <= WINDOW_SEC or start >= duration - WINDOW_SEC - 1e-6
    return templates.TemplateResponse(
        request,
        "view.html",
        {
            "rec": rec,
            "start_sec": round(start, 3),
            "window_sec": WINDOW_SEC,
            "prev_sec": round(max(0.0, start - WINDOW_SEC), 3),
            "next_sec": round(start + WINDOW_SEC, 3),
            "at_start": at_start,
            "at_end": at_end,
            "series": series,
            "overview": overview,
            "page_url": page_url,
            "download_url": download_url,
        },
    )


@app.on_event("startup")
def on_startup() -> None:
    DATA.mkdir(parents=True, exist_ok=True)
    init_db()
    if db_ready:
        log.info("PostgreSQL готов, файлы в %s", DATA)
    else:
        log.error("PostgreSQL недоступен: %s", db_error)


@app.post("/api/upload")
async def upload(
    patient_name: str = Form(...),
    device_id: str = Form(...),
    sample_rate: int = Form(...),
    file: UploadFile = File(...),
    x_device_token: str = Header(default=""),
):
    if x_device_token != device_token():
        raise HTTPException(401, "Неверный токен")
    if sample_rate <= 0 or sample_rate > 1000:
        raise HTTPException(400, "Неверная частота")
    original = (file.filename or "").lower()
    if original and not original.endswith(".csv"):
        raise HTTPException(400, "Нужен файл csv")

    patient = safe_token(patient_name, "patient")
    device = safe_token(device_id, "device")
    received = datetime.now().astimezone()
    dest = allocate_name(patient, received)
    partial = dest.with_suffix(".csv.part")
    newlines = 0
    size = 0
    try:
        with partial.open("wb") as out:
            while True:
                chunk = await file.read(65536)
                if not chunk:
                    break
                newlines += chunk.count(b"\n")
                size += len(chunk)
                out.write(chunk)
        rows = data_rows_from_newlines(partial, newlines)
        duration = rows / sample_rate
        partial.replace(dest)
    except OSError as exc:
        partial.unlink(missing_ok=True)
        raise HTTPException(500, f"Не сохранить файл: {exc}") from exc

    try:
        with connect() as conn:
            row = conn.execute(
                """
                INSERT INTO recordings
                    (patient_name, device_id, sample_rate, received_at,
                     filename, size_bytes, duration_sec)
                VALUES (%s, %s, %s, %s, %s, %s, %s)
                RETURNING id
                """,
                (patient, device, sample_rate, received, dest.name, size, duration),
            ).fetchone()
            conn.commit()
    except psycopg.Error as exc:
        dest.unlink(missing_ok=True)
        log.exception("Не записать историю")
        raise HTTPException(503, f"База недоступна: {exc}") from exc

    log.info("Принят %s (%s байт)", dest.name, size)
    return {"ok": True, "id": row["id"], "filename": dest.name}


@app.get("/")
def history(request: Request):
    rows = []
    error = ""
    try:
        with connect() as conn:
            rows = conn.execute(
                """
                SELECT id, patient_name, device_id, sample_rate, received_at,
                       filename, size_bytes, duration_sec
                FROM recordings
                ORDER BY received_at DESC
                """
            ).fetchall()
    except psycopg.Error as exc:
        error = str(exc)
    return templates.TemplateResponse(
        request,
        "history.html",
        {"rows": rows, "db_error": error},
    )


@app.get("/files")
def files_page(request: Request):
    known = {}
    error = ""
    try:
        with connect() as conn:
            for row in conn.execute("SELECT id, filename FROM recordings").fetchall():
                known[row["filename"]] = row["id"]
    except psycopg.Error as exc:
        error = str(exc)

    items = []
    for path in DATA.glob("*.csv"):
        stat = path.stat()
        items.append(
            {
                "name": path.name,
                "patient": patient_from_filename(path.name),
                "size": stat.st_size,
                "mtime": datetime.fromtimestamp(stat.st_mtime).strftime("%Y-%m-%d %H:%M:%S"),
                "recording_id": known.get(path.name),
            }
        )
    items.sort(key=lambda item: item["mtime"], reverse=True)
    return templates.TemplateResponse(
        request,
        "files.html",
        {"items": items, "db_error": error, "folder": str(DATA)},
    )


@app.get("/files/{filename}/download")
def download_loose(filename: str):
    path = csv_path(filename)
    return FileResponse(path, media_type="text/csv", filename=path.name)


@app.get("/files/{filename}")
def view_loose_file(
    request: Request,
    filename: str,
    start_sec: float = Query(0),
):
    path = csv_path(filename)
    try:
        with connect() as conn:
            row = conn.execute(
                "SELECT id FROM recordings WHERE filename = %s", (filename,)
            ).fetchone()
        if row:
            query = f"?start_sec={start_sec}" if start_sec else ""
            return RedirectResponse(f"/recordings/{row['id']}{query}", status_code=307)
    except psycopg.Error:
        pass
    received = datetime.fromtimestamp(path.stat().st_mtime).astimezone()
    rows = count_data_rows(path)
    rec = {
        "id": None,
        "patient_name": patient_from_filename(filename),
        "device_id": "файл с диска",
        "sample_rate": 50,
        "received_at": received,
        "filename": filename,
        "size_bytes": path.stat().st_size,
        "duration_sec": rows / 50,
    }
    return render_view(
        request,
        rec,
        start_sec,
        f"/files/{filename}",
        f"/files/{filename}/download",
    )


@app.get("/recordings/{recording_id}")
def view_recording(
    request: Request,
    recording_id: int,
    start_sec: float = Query(0),
):
    try:
        rec = recording_or_404(recording_id)
    except psycopg.Error as exc:
        raise HTTPException(503, str(exc)) from exc
    return render_view(
        request,
        rec,
        start_sec,
        f"/recordings/{recording_id}",
        f"/recordings/{recording_id}/download",
    )


@app.get("/api/recordings/{recording_id}/series")
def recording_series(
    recording_id: int,
    start_sec: float = Query(0),
    seconds: float = Query(WINDOW_SEC),
):
    rec = recording_or_404(recording_id)
    seconds = min(max(0.1, seconds), 30.0)
    path = csv_path(rec["filename"])
    duration = float(rec["duration_sec"])
    rate = int(rec["sample_rate"])
    start = clamp_start(start_sec, duration, seconds)
    points = read_samples(path, int(start * rate), max(1, int(seconds * rate)), 1)
    points["start_sec"] = start
    points["seconds"] = seconds
    points["duration_sec"] = duration
    points["sample_rate"] = rate
    return points


@app.get("/api/recordings/{recording_id}/overview")
def recording_overview(recording_id: int):
    rec = recording_or_404(recording_id)
    path = csv_path(rec["filename"])
    duration = float(rec["duration_sec"])
    rate = int(rec["sample_rate"])
    points = read_samples(path, 0, OVERVIEW_POINTS, overview_step(duration, rate))
    points["duration_sec"] = duration
    points["sample_rate"] = rate
    return points


@app.get("/recordings/{recording_id}/download")
def download_recording(recording_id: int):
    rec = recording_or_404(recording_id)
    path = csv_path(rec["filename"])
    return FileResponse(path, media_type="text/csv", filename=rec["filename"])
