FROM python:3.11-slim

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PIP_NO_CACHE_DIR=1

WORKDIR /app

RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ build-essential \
    && rm -rf /var/lib/apt/lists/*

COPY requirements.txt ./requirements.txt
RUN pip install --upgrade pip \
    && pip install -r requirements.txt

COPY . .

RUN g++ -std=c++17 -O2 loop_optimizer.cpp loop_optimizer_engine.cpp file_utils.cpp cli.cpp performance.cpp -o loop_optimizer.exe

EXPOSE 8501

CMD ["streamlit", "run", "optimizer_dashboard.py", "--server.address=0.0.0.0", "--server.port=8501"]