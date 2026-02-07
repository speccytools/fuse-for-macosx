.PHONY: 3rdparty audiofile libgcrypt FuseGenerator FuseImporter mbedtls clean-3rdparty list-teams fusex archive dist dmg clean

# Signing parameters (can be overridden from command line)
# Example: make 3rdparty DEVELOPMENT_TEAM="ABC123DEFG" CODE_SIGN_IDENTITY="Apple Development"
DEVELOPMENT_TEAM ?=
CODE_SIGN_IDENTITY ?=
CODE_SIGN_STYLE ?= Automatic
PROVISIONING_PROFILE_SPECIFIER ?=

# Archive path (can be overridden from command line)
# Example: make archive ARCHIVE_PATH=./build/FuseX.xcarchive
ARCHIVE_PATH ?= ./build/FuseX.xcarchive

# Automatically set CODE_SIGN_STYLE=Manual if Developer ID identity is specified
ifneq ($(CODE_SIGN_IDENTITY),)
ifeq ($(findstring Developer ID,$(CODE_SIGN_IDENTITY)),Developer ID)
	CODE_SIGN_STYLE := Manual
endif
endif

# Build common xcodebuild arguments for signing
XCODEBUILD_SIGN_ARGS :=
ifneq ($(DEVELOPMENT_TEAM),)
	XCODEBUILD_SIGN_ARGS += DEVELOPMENT_TEAM="$(DEVELOPMENT_TEAM)"
endif
ifneq ($(CODE_SIGN_IDENTITY),)
	XCODEBUILD_SIGN_ARGS += CODE_SIGN_IDENTITY="$(CODE_SIGN_IDENTITY)"
	XCODEBUILD_SIGN_ARGS += CODE_SIGN_IDENTITY_ALL="$(CODE_SIGN_IDENTITY)"
endif
ifneq ($(CODE_SIGN_STYLE),)
	XCODEBUILD_SIGN_ARGS += CODE_SIGN_STYLE="$(CODE_SIGN_STYLE)"
endif
ifneq ($(PROVISIONING_PROFILE_SPECIFIER),)
	XCODEBUILD_SIGN_ARGS += PROVISIONING_PROFILE_SPECIFIER="$(PROVISIONING_PROFILE_SPECIFIER)"
endif

all: 3rdparty

fusepb:
	cd fusepb && make clean && make

3rdparty: audiofile libgcrypt FuseGenerator FuseImporter mbedtls

audiofile:
	@echo "Building audiofile Framework..."
	cd 3rdparty/audiofile && \
	xcodebuild -project audiofile.xcodeproj \
		-target "audiofile Framework" \
		-configuration Deployment \
		SYMROOT=build \
		BUILD_DIR=build \
		CONFIGURATION_BUILD_DIR=build/Deployment \
		$(XCODEBUILD_SIGN_ARGS)

libgcrypt:
	@echo "Building gcrypt Framework..."
	cd 3rdparty/libgcrypt && \
	xcodebuild -project libgcrypt.xcodeproj \
		-target "gcrypt Framework" \
		-configuration Deployment \
		SYMROOT=build \
		BUILD_DIR=build \
		CONFIGURATION_BUILD_DIR=build/Deployment \
		$(XCODEBUILD_SIGN_ARGS)

FuseGenerator:
	@echo "Building FuseGenerator Framework..."
	cd 3rdparty/FuseGenerator && \
	xcodebuild -project FuseGenerator.xcodeproj \
		-target "FuseGenerator" \
		-configuration Deployment \
		SYMROOT=build \
		BUILD_DIR=build \
		CONFIGURATION_BUILD_DIR=build/Deployment \
		$(XCODEBUILD_SIGN_ARGS)

FuseImporter:
	@echo "Building FuseImporter Framework..."
	cd 3rdparty/FuseImporter && \
	xcodebuild -project FuseImporter.xcodeproj \
		-target "FuseImporter" \
		-configuration Deployment \
		SYMROOT=build \
		BUILD_DIR=build \
		CONFIGURATION_BUILD_DIR=build/Deployment \
		$(XCODEBUILD_SIGN_ARGS)

mbedtls:
	@echo "Building mbedtls Framework..."
	cd 3rdparty/mbedtls && \
	xcodebuild -project mbedtls.xcodeproj \
		-target "mbedtls" \
		-configuration Deployment \
		SYMROOT=build \
		BUILD_DIR=build \
		CONFIGURATION_BUILD_DIR=build/Deployment \
		$(XCODEBUILD_SIGN_ARGS)

list-teams:
	@echo "Available code signing identities and development teams:"
	@echo ""
	@if security find-identity -v -p codesigning 2>/dev/null | grep -q "valid identities found"; then \
		echo "Unique Development Team IDs:"; \
		security find-identity -v -p codesigning 2>/dev/null | \
		grep -oE '\([A-Z0-9]{10}\)' | \
		sed 's/[()]//g' | \
		sort -u | \
		while read team; do \
			echo "  $$team"; \
		done; \
		echo ""; \
		echo "Full identity list:"; \
		security find-identity -v -p codesigning 2>/dev/null; \
	else \
		echo "No code signing identities found. Make sure you have certificates installed in your keychain."; \
	fi

fusex: 3rdparty fusepb
	@echo "Building FuseX app..."
	cd fusepb && \
	xcodebuild -project FuseX.xcodeproj \
		-target FuseX \
		-configuration Deployment \
		SYMROOT=build \
		BUILD_DIR=build \
		CONFIGURATION_BUILD_DIR=build/Deployment \
		$(XCODEBUILD_SIGN_ARGS)

archive: 3rdparty fusepb
	@echo "Archiving FuseX app..."
	@mkdir -p build
	cd fusepb && \
	xcodebuild archive \
		-project FuseX.xcodeproj \
		-scheme FuseX \
		-configuration Deployment \
		-archivePath "$(CURDIR)/$(ARCHIVE_PATH)" \
		SYMROOT=build \
		BUILD_DIR=build \
		CONFIGURATION_BUILD_DIR=build/Deployment \
		$(XCODEBUILD_SIGN_ARGS)
	@echo "Archive created at: $(ARCHIVE_PATH)"

dist:
	@echo "Checking for dist/FuseX.app..."
	@if [ ! -d "dist/FuseX.app" ]; then \
		echo "Error: dist/FuseX.app not found. Please copy FuseX.app to dist/ first."; \
		exit 1; \
	fi
	@echo "dist/FuseX.app found."

dmg: dist
	@echo "Creating FuseX.dmg..."
	@if ! command -v create-dmg >/dev/null 2>&1; then \
		echo "Error: create-dmg is not installed. Install it with: brew install create-dmg"; \
		exit 1; \
	fi
	@rm -f FuseX.dmg
	@if create-dmg \
		--volname "FuseX" \
		--window-pos 200 120 \
		--window-size 600 300 \
		--icon-size 100 \
		--icon "FuseX.app" 175 120 \
		--hide-extension "FuseX.app" \
		--app-drop-link 425 120 \
		FuseX.dmg \
		dist/; then \
		if [ -f "FuseX.dmg" ]; then \
			echo "DMG created successfully: FuseX.dmg"; \
		else \
			echo "Error: DMG file was not created"; \
			exit 1; \
		fi \
	else \
		echo "Error: create-dmg failed"; \
		exit 1; \
	fi

clean-3rdparty:
	@echo "Cleaning 3rdparty build artifacts..."
	rm -rf 3rdparty/audiofile/build
	rm -rf 3rdparty/libgcrypt/build
	rm -rf 3rdparty/FuseGenerator/build
	rm -rf 3rdparty/FuseImporter/build
	rm -rf 3rdparty/mbedtls/build

clean: clean-3rdparty
	@echo "Cleaning FuseX build artifacts..."
	rm -rf fusepb/build
	rm -rf build
	rm -rf dist
	rm -f FuseX.dmg
