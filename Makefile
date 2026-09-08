# ==============================================================================
# Makefile for Slint-cflat (MinGW / Windows Environment)
# ==============================================================================

# バージョン定義
VERSION ?= 1.17.1

# ビルド対象のターゲットトリプル (Make変数で環境を切り替え可能)
TARGET ?= x86_64-pc-windows-gnu

# Cargo出力ディレクトリと配布フォルダ定義
TARGET_DIR = target/$(TARGET)/release
DIST_DIR   = dist
PKG_NAME   = slint-cflat-$(VERSION)
PKG_DIR    = $(DIST_DIR)/$(PKG_NAME)
ZIP_FILE   = $(DIST_DIR)/$(PKG_NAME).zip

.PHONY: all lib package clean

all: lib

# ------------------------------------------------------------------------------
# 1. Rustライブラリのコンパイル (make lib)
# ------------------------------------------------------------------------------
lib:
	cargo build --release --target $(TARGET)

# ------------------------------------------------------------------------------
# 2. 配布用アーカイブの生成 (make package)
# ------------------------------------------------------------------------------
package: lib
	@echo "Creating distribution package: $(PKG_NAME)..."
	# 出力先ディレクトリの作成
	mkdir -p $(PKG_DIR)

	# DLL およびインポートライブラリ (.dll.a) のコピー
	cp $(TARGET_DIR)/*.dll $(PKG_DIR)/
	cp $(TARGET_DIR)/*.dll.a $(PKG_DIR)/

	# include ディレクトリ全体のコピー
	cp -r include $(PKG_DIR)/

	# ドキュメントおよびライセンスファイルのコピー (大文字 LICENSE と README)
	cp LICENSE $(PKG_DIR)/
	@if [ -f README.markdown ]; then \
		cp README.markdown $(PKG_DIR)/; \
	elif [ -f README.md ]; then \
		cp README.md $(PKG_DIR)/; \
	fi

	# Windows標準のPowerShellまたはtarを使用してZIPアーカイブを圧縮
	@echo "Compressing $(ZIP_FILE)..."
	powershell -NoProfile -Command \
		"Compress-Archive -Path '$(PKG_DIR)' -DestinationPath '$(ZIP_FILE)' -Force"

	@echo "Package created successfully: $(ZIP_FILE)"

# ------------------------------------------------------------------------------
# クリーンアップ
# ------------------------------------------------------------------------------
clean:
	cargo clean
	rm -rf $(DIST_DIR)