#!/bin/sh
protoc --python_out=. --proto_path=../../core/proto ../../core/proto/messages.proto
