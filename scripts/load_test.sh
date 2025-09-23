#!/usr/bin/env bash

URL="http://127.0.0.1:8080/"
TOTAL=100000
CONCURRENCY=100000  # simulate 1000 users

# Create a list of requests
seq $TOTAL | xargs -n1 -P $CONCURRENCY -I{} curl -s -o /dev/null -w "%{http_code}\n" "$URL"
