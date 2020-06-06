FROM alpine:3.11
RUN apk add -X http://dl-cdn.alpinelinux.org/alpine/edge/testing g++ poco-dev
WORKDIR /app
ADD server.cpp server.cpp
RUN g++ -o server server.cpp -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON

FROM alpine:3.11
RUN apk add --no-cache -X http://dl-cdn.alpinelinux.org/alpine/edge/testing poco
WORKDIR /app
COPY --from=0 /app/server server
COPY debug healthz error2 ./
CMD ["./server"]
