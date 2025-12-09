.PHONY: 3rdparty audiofile libgcrypt clean-3rdparty

3rdparty: audiofile libgcrypt

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

clean-3rdparty:
	@echo "Cleaning 3rdparty build artifacts..."
	rm -rf 3rdparty/audiofile/build
	rm -rf 3rdparty/libgcrypt/build
