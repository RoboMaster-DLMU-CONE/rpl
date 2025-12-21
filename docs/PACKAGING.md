# RPL 打包指南

本项目支持将 **RPL 库**（header-only）打包为安装包。

## 项目结构

- **RPL** - RoboMaster 数据包序列化/反序列化库（header-only C++20 库）

## 打包 RPL 库（开发包）

RPL 是一个 header-only 库，打包后供其他开发者在项目中使用。

### 构建和打包步骤

```bash
# 清理并创建构建目录
rm -rf build && mkdir build && cd build

# 配置项目
cmake ..

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

**注意**: RPL 库的所有依赖已经打包在内，包括：

- tl::expected (已打包)
- cppcrc (已打包为头文件)
- frozen (已打包)
- ringbuffer (RPL 内部实现，已包含)

不需要手动安装任何额外依赖。

---


---

## 支持的包格式

### RPL 库包格式

- ✅ **TGZ** (.tar.gz) - 通用压缩包
- ✅ **ZIP** (.zip) - 通用压缩包
- ✅ **DEB** (.deb) - Debian/Ubuntu 包
- ✅ **RPM** (.rpm) - RedHat/Fedora/CentOS 包


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

下载并安装 [WiX Toolset](https://wixtoolset.org/)

---

## 包版本信息

所有包的版本号由 `CMakeLists.txt` 中的 `project(rpl VERSION 0.1.0)` 定义。

修改版本号后，重新运行 CMake 配置即可生成新版本的包。

---

## 自动化打包（GitHub Actions）

本项目已配置自动化打包工作流，可以自动生成和发布 RPL 库的安装包。

### 触发方式

1. **创建版本标签** - 推送符合 `v*` 格式的标签（如 `v0.1.0`）
2. **手动触发** - 在 GitHub Actions 界面手动运行相应的工作流：
   - "Package RPL" - RPL 库打包

### 自动生成的包

工作流会自动生成以下格式的包：

**RPL 库包（Linux）:**
- `rpl-{version}-Linux.tar.gz` - TGZ 压缩包
- `rpl-{version}-Linux.zip` - ZIP 压缩包
- `rpl-{version}-Linux.deb` - Debian/Ubuntu 包
- `rpl-{version}-Linux.rpm` - RPM 包

### 包的获取

- **标签触发**: 包会自动上传到对应 Release 的 Assets 中
- **手动触发**: 包可在工作流运行页面的 Artifacts 中下载（保留 90 天）

### 手动创建和发布版本

```bash
# 1. 更新版本号
# 编辑 CMakeLists.txt，修改 project(rpl VERSION x.y.z)

# 2. 提交版本变更
git add CMakeLists.txt
git commit -m "Bump version to x.y.z"
git push

# 3. 创建并推送标签
git tag vx.y.z
git push origin vx.y.z

# 4. GitHub Actions 会自动构建并发布包到 Release
```

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

