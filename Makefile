.PHONY: 3rdparty audiofile libgcrypt FuseGenerator FuseImporter clean-3rdparty

3rdparty: audiofile libgcrypt FuseGenerator  FuseImporter

audiofile:
	@echo "Building audiofile Framework..."
	cd 3rdparty/audiofile && \
	xcodebuild -project audiofile.xcodeproj \
		-target "audiofile Framework" \
		-configuration Deployment \
		SYMROOT=build \
		BUILD_DIR=build \
		CONFIGURATION_BUILD_DIR=build/Deployment

libgcrypt:
	@echo "Building gcrypt Framework..."
	cd 3rdparty/libgcrypt && \
	xcodebuild -project libgcrypt.xcodeproj \
		-target "gcrypt Framework" \
		-configuration Deployment \
		SYMROOT=build \
		BUILD_DIR=build \
		CONFIGURATION_BUILD_DIR=build/Deployment

FuseGenerator:
	@echo "Building FuseGenerator Framework..."
	cd 3rdparty/FuseGenerator && \
	xcodebuild -project FuseGenerator.xcodeproj \
		-target "FuseGenerator" \
		-configuration Deployment \
		SYMROOT=build \
		BUILD_DIR=build \
		CONFIGURATION_BUILD_DIR=build/Deployment

FuseImporter:
	@echo "Building FuseImporter Framework..."
	cd 3rdparty/FuseImporter && \
	xcodebuild -project FuseImporter.xcodeproj \
		-target "FuseImporter" \
		-configuration Deployment \
		SYMROOT=build \
		BUILD_DIR=build \
		CONFIGURATION_BUILD_DIR=build/Deployment

clean-3rdparty:
	@echo "Cleaning 3rdparty build artifacts..."
	rm -rf 3rdparty/audiofile/build
	rm -rf 3rdparty/libgcrypt/build
	rm -rf 3rdparty/FuseGenerator/build
	rm -rf 3rdparty/FuseImporter/build
