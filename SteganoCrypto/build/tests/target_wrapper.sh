#!/bin/sh
PATH=/C/msys64/ucrt64/bin:$PATH
export PATH
QT_PLUGIN_PATH=/C/msys64/ucrt64/share/qt6/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}
export QT_PLUGIN_PATH
exec "$@"
