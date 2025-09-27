FROM ubuntu:22.04 AS development

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libpqxx-dev \
    libhiredis-dev \
    curl \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt ./
COPY src/ ./src/

RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

EXPOSE 8002

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:8002/health || exit 1

CMD ["./build/bill-service"]

FROM ubuntu:22.04 AS production

RUN apt-get update && apt-get install -y \
    libpqxx-7.7 \
    libhiredis0.14 \
    curl \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -r -s /bin/false billservice

WORKDIR /app

COPY --from=development /app/build/bill-service ./

RUN chown billservice:billservice /app/bill-service

USER billservice

EXPOSE 8002

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:8002/health || exit 1

CMD ["./bill-service"]