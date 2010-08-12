#!/bin/sh

pod2man -n bt-adapter -c "bluez-tools" -r "" man/bt-adapter.pod > ../src/bt-adapter.1
pod2man -n bt-agent -c "bluez-tools" -r "" man/bt-agent.pod > ../src/bt-agent.1
pod2man -n bt-device -c "bluez-tools" -r "" man/bt-device.pod > ../src/bt-device.1
