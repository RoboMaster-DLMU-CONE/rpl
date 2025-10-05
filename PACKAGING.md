# RPL 和 RPLC 打包指南

本项目现在支持将 **RPL 库**（header-only）和 **RPLC 工具**（命令行工具）分别打包为不同的安装包。

## 项目结构

- **RPL** - RoboMaster 数据包序列化/反序列化库（header-only C++20 库）
- **RPLC** - RPL 命令行工具，用于解析 JSON 并转换为 RPL 包格式

## 打包 RPL 库（开发包）

RPL 是一个 header-only 库，打包后供其他开发者在项目中使用。

### 构建和打包步骤

```bash
# 清理并创建构建目录
rm -rf build && mkdir build && cd build

# 配置项目（不构建 RPLC）
cmake .. -DBUILD_RPLC=OFF

# 构建
make -j$(nproc)

# 生成不同格式的安装包
cpack -G TGZ    # 生成 .tar.gz 包
cpack -G ZIP    # 生成 .zip 包
cpack -G DEB    # 生成 .deb 包（需要 dpkg-dev）
cpack -G RPM    # 生成 .rpm 包（需要 rpmbuild）
```

### RPL 库包内容

- **头文件**: `include/RPL/` 目录下的所有 `.hpp` 文件
- **CMake 配置**: `lib/cmake/rpl/rplConfig.cmake` 和版本文件
- **包名称**: `librpl-dev` (Debian) 或 `rpl-devel` (RPM)

### 使用已安装的 RPL 库

安装 RPL 开发包后，在你的项目中：

```cmake
find_package(rpl REQUIRED)
target_link_libraries(your_target PRIVATE rpl::rpl)
```

**注意**: 使用 RPL 库时，你还需要手动安装以下依赖：

- tl::expected
- cppcrc
- frozen
- ringbuffer

---

## 打包 RPLC 工具（可执行程序）

RPLC 是一个命令行工具，打包后供最终用户直接使用。

### 构建和打包步骤

```bash
# 清理并创建构建目录
rm -rf build && mkdir build && cd build

# 配置项目（构建 RPLC）
cmake .. -DBUILD_RPLC=ON

# 构建
make -j$(nproc)

# 生成不同格式的安装包
cpack -G TGZ    # 生成 .tar.gz 包
cpack -G ZIP    # 生成 .zip 包
cpack -G DEB    # 生成 .deb 包（需要 dpkg-dev）
cpack -G RPM    # 生成 .rpm 包（需要 rpmbuild）
cpack -G NSIS   # 生成 .exe 安装程序（仅 Windows，需要 NSIS）
```

### RPLC 工具包内容

- **可执行文件**: `bin/rplc`
- **包名称**: `rplc`
- **分类**: Debian (utils) / RPM (Development/Tools)

### 使用已安装的 RPLC 工具

安装后，直接在命令行使用：

```bash
rplc --help
```

---

## Windows 平台打包（RPLC）

在 Windows 上生成 NSIS 安装程序：

```bash
# 使用 Visual Studio 生成器
cmake .. -G "Visual Studio 17 2022" -DBUILD_RPLC=ON

# 构建 Release 版本
cmake --build . --config Release

# 生成 NSIS 安装程序（需要先安装 NSIS）
cpack -G NSIS
```

这会生成一个 `rplc-0.1.0-win64.exe` 安装程序，支持：

- 自动添加到系统 PATH
- 卸载前自动删除旧版本
- 标准的 Windows 安装向导

---

## 支持的包格式

### RPL 库包格式

- ✅ **TGZ** (.tar.gz) - 通用压缩包
- ✅ **ZIP** (.zip) - 通用压缩包
- ✅ **DEB** (.deb) - Debian/Ubuntu 包
- ✅ **RPM** (.rpm) - RedHat/Fedora/CentOS 包

### RPLC 工具包格式

- ✅ **TGZ** (.tar.gz) - 通用压缩包
- ✅ **ZIP** (.zip) - 通用压缩包
- ✅ **DEB** (.deb) - Debian/Ubuntu 包
- ✅ **RPM** (.rpm) - RedHat/Fedora/CentOS 包
- ✅ **NSIS** (.exe) - Windows 安装程序

---

## 安装依赖工具

### Linux (Fedora/RHEL)

```bash
sudo dnf install dpkg-devel rpm-build
```

### Linux (Debian/Ubuntu)

```bash
sudo apt-get install dpkg-dev rpm
```

### Windows

下载并安装 [NSIS](https://nsis.sourceforge.io/)

---

## 包版本信息

所有包的版本号由 `CMakeLists.txt` 中的 `project(rpl VERSION 0.1.0)` 定义。

修改版本号后，重新运行 CMake 配置即可生成新版本的包。

---

## 故障排除

### CPack 找不到生成器

确保安装了相应的打包工具（dpkg-dev, rpmbuild, NSIS）。

### "package" 目标不存在

使用 `cpack -G <GENERATOR>` 命令代替 `make package`。

### RPM 包生成失败

安装 `rpm-build` 工具：

```bash
sudo dnf install rpm-build  # Fedora/RHEL
sudo apt-get install rpm    # Debian/Ubuntu
```

### DEB 包架构显示为 i386

安装 `dpkg-dev` 工具以获得正确的架构检测。

