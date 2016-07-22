#
# Copyright (C) 2011-2016 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#

include ../../../buildenv.mk

SIM_DIR  := $(CUR_DIR)/..

CPPFLAGS += $(ADDED_INC) # for unbuntu 11 and later version
CPPFLAGS += -I$(COMMON_DIR)/inc/           \
            -I$(COMMON_DIR)/inc/internal   \
            -I$(COMMON_DIR)/inc/tlibc      \
            -I$(SIM_DIR)/assembly/         \
            -I$(SIM_DIR)/assembly/linux    \
            -I$(SIM_DIR)/uinst/            \
            -I$(LINUX_SDK_DIR)/selib

CXXFLAGS += $(ENCLAVE_CXXFLAGS) -Werror -fno-exceptions -fno-rtti

LIBSESIMU_T := libsesimu_t.a

OBJS := deriv.o t_instructions.o

.PHONY:all
all: $(LIBSESIMU_T)

$(LIBSESIMU_T): $(OBJS)
	$(AR) rcs $@ $^

# Explicitly disable optimization for 't_instructions.cpp',
# since the '_SE3' function has assumptions on stack layout.
t_instructions.o: CXXFLAGS += -O0

.PHONY: clean
clean:
	@$(RM) $(OBJS) $(LIBSESIMU_T)