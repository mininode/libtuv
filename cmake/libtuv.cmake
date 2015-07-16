# Copyright 2015 Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 2.8)

set(COMMON_SRCFILES "${SOURCE_ROOT}/uv_handle.cpp"
                    "${SOURCE_ROOT}/uv_loop.cpp"
                    "${SOURCE_ROOT}/uv_idle.cpp"
                    "${SOURCE_ROOT}/uv_run.cpp"
                    "${SOURCE_ROOT}/uv_timer.cpp"
                    "${SOURCE_ROOT}/uv_threadpool.cpp"
                    "${SOURCE_ROOT}/uv_req.cpp"
                    "${SOURCE_ROOT}/uv_fs.cpp"
                    "${SOURCE_ROOT}/uv_async.cpp"
                    "${SOURCE_ROOT}/uv_util.cpp"
                    )

set(LIB_TUV_SRCFILES
    ${COMMON_SRCFILES}
    ${PLATFORM_SRCFILES}
)

set(LIB_TUV_INCDIRS
    ${TARGET_INC}
    ${INCLUDE_ROOT}
    ${SOURCE_ROOT}
    ${SOURCE_ROOT}/${TUV_PLATFORM_PATH}
)

# build tuv library
set(TARGETLIBNAME tuv)
add_library(${TARGETLIBNAME} ${LIB_TUV_SRCFILES})
target_include_directories(${TARGETLIBNAME} PUBLIC ${LIB_TUV_INCDIRS})
set_target_properties(${TARGETLIBNAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${LIB_OUT}"
    LIBRARY_OUTPUT_DIRECTORY "${LIB_OUT}"
    RUNTIME_OUTPUT_DIRECTORY "${BIN_OUT}")