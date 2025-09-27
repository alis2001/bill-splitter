FROM alpine:latest AS development
RUN apk add --no-cache curl
EXPOSE 8002
CMD ["sh", "-c", "while true; do echo 'HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"ok\",\"service\":\"bill-service\"}' | nc -l -p 8002; done"]
