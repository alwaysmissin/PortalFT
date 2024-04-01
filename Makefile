CC := gcc
CFLAGS := -Iinclude
LDFLAGS := -lreadline
SRCDIR := src
BUILDDIR := build
OBJDIR := $(BUILDDIR)/objs
TARGET := $(BUILDDIR)/PortalFT

# 获取源文件列表
SRCS := $(wildcard $(SRCDIR)/*.c)
# 生成对应的目标文件列表
OBJS := $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
ARGS := -l $(BUILDDIR)/PortalFT.log

# 默认目标，编译可执行文件
all: $(TARGET)

run: $(TARGET)
	$(TARGET) $(ARGS)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJS) | $(BUILDDIR)
	$(CC) $^ $(LDFLAGS) -o $@ -fsanitize=address

# 编译每个源文件生成对应的目标文件
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -fsanitize=address

# 创建 build 目录
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# 创建 objs 目录
$(OBJDIR):
	mkdir -p $(OBJDIR)

# 清理生成的文件
clean:
	$(RM) -r $(BUILDDIR)

.PHONY: all clean
