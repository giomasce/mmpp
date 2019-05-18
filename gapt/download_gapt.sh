#!/bin/bash

set -e

wget -q --continue https://www.logic.at/gapt/downloads/gapt-2.14.tar.gz
tar zxf gapt-2.14.tar.gz
ln gapt-2.14/gapt-2.14.jar
