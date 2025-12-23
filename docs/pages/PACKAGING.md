# RPL 打包指南


RPL 是一个 header-only 库，可以在打包后供其他开发者在项目中使用。

## 构建和打包步骤

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

## RPL 库包内容

- **头文件**: `include/RPL/` 目录下的所有 `.hpp` 文件
- **CMake 配置**: `lib/cmake/rpl/rplConfig.cmake` 和版本文件
- **包名称**: `librpl-dev` (Debian) 或 `rpl-devel` (RPM)

## 使用已安装的 RPL 库

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


