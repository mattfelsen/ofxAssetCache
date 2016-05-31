//
//  ofxAssetCache.cpp
//  ofxAssetCache
//
//  Created by Matt Felsen on 12/7/15.
//
//

#include "ofxAssetCache.h"

ofxAssetCache::ofxAssetCache() {
	emptyImage = make_shared<ofImage>();
	emptyImage->allocate(1, 1, OF_IMAGE_COLOR_ALPHA);
	addImage("", emptyImage->getPixels());

	emptyTexture = make_shared<ofTexture>();
	emptyTexture->allocate(1, 1, GL_RGBA);
	addTexture("", emptyImage->getPixels());

	emptyVideo = make_shared<ofVideoPlayer>();
	emptyHapVideo = make_shared<ofxHapPlayer>();

	startThread();
	ofAddListener(ofEvents().update, this, &ofxAssetCache::update);
}

ofxAssetCache::~ofxAssetCache() {
	stopThread();
	ofRemoveListener(ofEvents().update, this, &ofxAssetCache::update);
}

void ofxAssetCache::threadedFunction() {
	while (isThreadRunning()) {

		// Check for items in the CPU loader queue
		// Read data from the queued item's filename into a buffer and then
		// decompress the image data to an ofPixels object (either with
		// turbo-jpeg if applicable, or FreeImage otherwise
		// If successful, move it to the GPU loader queue which happens on the
		// main thread
		while (cpuQueue.size()) {
			auto& q = cpuQueue.front();
			if (q.pixels.isAllocated()) continue;

			ofLogVerbose("Assets") << "loading (cpu) " << q.filename;

			if (loadToPixels(q.filename, q.pixels))
				gpuQueue.push_back(q);

			cpuQueue.pop_front();
		}

		ofSleepMillis(1);
	}
}

void ofxAssetCache::update(ofEventArgs& args) {

	// Check for items in the GPU loader queue
	// Data is loaded from files and stored in an ofPixels object in the thread
	// above. Here the pixel data gets uploaded to the GPU via an ofImage or
	// ofTexture, depending on the desired type
	// Only load 1 item per frame to prevent UI blocking
	if (gpuQueue.size()) {
		auto q = gpuQueue.front();
		if (!q.pixels.isAllocated()) return;

		if (q.type == IMAGE)
			addImage(q.name, q.pixels, q.mipmaps);

		if (q.type == TEXTURE)
			addTexture(q.name, q.pixels, q.mipmaps);

		gpuQueue.pop_front();
	}

}

shared_ptr<ofImage> ofxAssetCache::image(const string &name) {
	// TODO Cache empty image so you only get a warning the first time
	// you try to access it and not every frame trying to draw it?
	if (!exists(images, name)) {
		ofLogError("Assets") << "no such image: " << name;
		return emptyImage;
	}

	return images[name];
}
ofImage* ofxAssetCache::imagePointer(const string& name) {
	return image(name).get();
}

shared_ptr<ofTexture> ofxAssetCache::texture(const string &name) {
	// TODO Cache empty texture so you only get a warning the first time
	// you try to access it and not every frame trying to draw it?
	if (!exists(textures, name)) {
		ofLogError("Assets") << "no such texture: " << name;
		return emptyTexture;
	}

	return textures[name];
}
ofTexture* ofxAssetCache::texturePointer(const string& name) {
	return texture(name).get();
}

shared_ptr<ofVideoPlayer> ofxAssetCache::video(const string &name) {
	// TODO Cache empty texture so you only get a warning the first time
	// you try to access it and not every frame trying to draw it?
	if (!exists(videos, name)) {
		ofLogError("Assets") << "no such video: " << name;
		return emptyVideo;
	}

	return videos[name];
}
ofVideoPlayer* ofxAssetCache::videoPointer(const string& name) {
	return video(name).get();
}

shared_ptr<ofxHapPlayer> ofxAssetCache::hapVideo(const string &name) {
	// TODO Cache empty texture so you only get a warning the first time
	// you try to access it and not every frame trying to draw it?
	if (!exists(hapVideos, name)) {
		ofLogError("Assets") << "no such Hap video: " << name;
		return emptyHapVideo;
	}

	return hapVideos[name];
}
ofxHapPlayer* ofxAssetCache::hapVideoPointer(const string& name) {
	return hapVideo(name).get();
}

shared_ptr<ofShader> ofxAssetCache::shader(const string &name) {
	if (!exists(shaders, name)) {
		ofLogError("Assets") << "no such shader: " << name;
		return nullptr;
	}

	return shaders[name];
}

/*
 shared_ptr<ofxImageSequence> ofxAssetCache::sequence(const string &name) {
	if (!exists(sequences, name)) {
 ofLogError("Assets") << "no such sequence: " << name;
 return nullptr;
	}

	return sequences[name];
 }
 */

bool ofxAssetCache::loadToPixels(const string& file, ofPixels& pix) {
	if (!ofFile(file).exists()) {
		ofLogError("Assets") << "loadToPixels: file does not exist: " << file;
		return false;
	}

	string ext = ofToLower(ofFile(file).getExtension());

	/*
	 if (ext == "jpg" || ext == "jpeg")
		turbo.load(file, pix);
	 else
	 */
	ofLoadImage(pix, file);

	if (!pix.isAllocated()) {
		ofLogError("Assets") << "loadToPixels: failed to load: " << file;
		return false;
	} else {
		return true;
	}
}

bool ofxAssetCache::addImage(const string &name, const string &filename, bool mipmaps) {
	string file = filename.empty() ? name : filename;

	if (file.empty()) {
		ofLogVerbose("Assets") << "addImage: filename is empty!";
		return false;
	}
	if (exists(images, name)) {
		ofLogVerbose("Assets") << "addImage: skipping: " << name;
		return true;
	}

	ofPixels pix;
	loadToPixels(file, pix);

	return addImage(name, pix, mipmaps);
}

bool ofxAssetCache::addImage(const string &name, ofPixels& pix, bool mipmaps) {
	if (exists(images, name)) {
		ofLogVerbose("Assets") << "addImage: skipping: " << name;
		return true;
	}

	if (!pix.isAllocated()) {
		ofLogVerbose("Assets") << "addImage: pixels not allocated! " << name;
		return false;
	}

	ofLogVerbose("Assets") << "loading (gpu) " << name;
	auto image = make_shared<ofImage>();

	if (mipmaps)
		image->getTexture().enableMipmap();

	image->setFromPixels(pix);

	if (!image->isAllocated()) {
		ofLogError("Assets") << "addImage: failed to load: " << name;
		return false;
	}

	images[name] = image;
	return true;
}

void ofxAssetCache::addImageAsync(const string &name, const string &filename, bool mipmaps) {
	string file = filename.empty() ? name : filename;

	if (file.empty()) {
		ofLogVerbose("Assets") << "addImageAsync: filename is empty!";
		return false;
	}
	if (exists(images, name)) {
		ofLogVerbose("Assets") << "addImageAsync: skipping: " << name;
		return true;
	}

	QueuedItem q;
	q.type = IMAGE;
	q.name = name;
	q.filename = file;
	q.mipmaps = mipmaps;

	cpuQueue.push_back(q);
}

bool ofxAssetCache::addTexture(const string &name, const string &filename, bool mipmaps) {
	string file = filename.empty() ? name : filename;

	if (file.empty()) {
		ofLogVerbose("Assets") << "addTexture: filename is empty!";
		return false;
	}
	if (exists(textures, name)) {
		ofLogVerbose("Assets") << "addTexture: skipping: " << name;
		return true;
	}

	ofPixels pix;
	loadToPixels(file, pix);

	return addTexture(name, pix, mipmaps);
}

bool ofxAssetCache::addTexture(const string &name, ofPixels& pix, bool mipmaps) {
	if (exists(textures, name)) {
		ofLogVerbose("Assets") << "addTexture: skipping: " << name;
		return true;
	}

	if (!pix.isAllocated()) {
		ofLogVerbose("Assets") << "addTexture: pixels not allocated! " << name;
		return false;
	}

	ofLogVerbose("Assets") << "loading (gpu) " << name;
	auto texture = make_shared<ofTexture>();

	if (mipmaps)
		texture->enableMipmap();

	texture->loadData(pix);

	if (!texture->isAllocated()) {
		ofLogError("Assets") << "addTexture: failed to load: " << name;
		return false;
	}

	textures[name] = texture;
	return true;
}

void ofxAssetCache::addTextureAsync(const string &name, const string &filename, bool mipmaps) {
	string file = filename.empty() ? name : filename;

	if (file.empty()) {
		ofLogVerbose("Assets") << "addTextureAsync: filename is empty!";
		return false;
	}
	if (exists(textures, name)) {
		ofLogVerbose("Assets") << "addTextureAsync: skipping: " << name;
		return true;
	}


	QueuedItem q;
	q.type = TEXTURE;
	q.name = name;
	q.filename = file;
	q.mipmaps = mipmaps;

	cpuQueue.push_back(q);
}

bool ofxAssetCache::addVideo(const string &name, const string &filename) {
	string file = filename.empty() ? name : filename;

	if (file.empty()) {
		ofLogVerbose("Assets") << "addVideo: filename is empty!";
		return false;
	}
	if (exists(videos, name)) {
		ofLogVerbose("Assets") << "addVideo: skipping: " << name;
		return true;
	}

	ofLogVerbose("Assets") << "loading " << file;
	auto video = make_shared<ofVideoPlayer>();

	if (!video->load(file)) {
		ofLogError("Assets") << "addVideo: failed to load: " << file;
		return false;
	}

	videos[name] = video;
	return true;
}

bool ofxAssetCache::addHapVideo(const string &name, const string &filename) {
	string file = filename.empty() ? name : filename;

	if (file.empty()) {
		ofLogVerbose("Assets") << "addHapVideo: filename is empty!";
		return false;
	}
	if (exists(hapVideos, name)) {
		ofLogVerbose("Assets") << "addHapVideo: skipping: " << name;
		return true;
	}

	ofLogVerbose("Assets") << "loading " << file;
	auto video = make_shared<ofxHapPlayer>();

	if (!video->load(file)) {
		ofLogError("Assets") << "addHapVideo: failed to load: " << file;
		return false;
	}

	hapVideos[name] = video;
	return true;
}

bool ofxAssetCache::addShader(const string &name, const string& filename) {
	string file = filename.empty() ? name : filename;

	if (file.empty()) {
		ofLogVerbose("Assets") << "addShader: filename is empty!";
		return false;
	}
	if (exists(shaders, name)) {
		ofLogVerbose("Assets") << "addShader: skipping: " << name;
		return true;
	}

	ofLogVerbose("Assets") << "adding shader: " << name;

	auto shader = make_shared<ofShader>();
	auto fileHandle = ofFile(file);

	if (fileHandle.getExtension().empty()) {
		shader->load(file);
	} else if (fileHandle.getExtension() == "frag") {
		shader->setupShaderFromFile(GL_FRAGMENT_SHADER, file);
	} else if (fileHandle.getExtension() == "vert") {
		shader->setupShaderFromFile(GL_VERTEX_SHADER, file);
	} else {
		ofLogError("Assets") << "don't know how to load shader: " << file;
		return false;
	}
	shader->linkProgram();

	shaders[name] = shader;
	return true;
}

/*
 bool ofxAssetCache::addSequence(const string &name, const string& folderName) {
	string folder = folderName.empty() ? name : folderName;

	if (folder.empty()) {
 ofLogVerbose("Assets") << "addSequence: folder name is empty!";
 return false;
	}
	if (exists(sequences, name)) {
 ofLogVerbose("Assets") << "addSequence: skipping: " << name;
 return true;
	}

	ofLogVerbose("Assets") << "adding sequence: " << name;
	auto sequence = make_shared<ofxImageSequence>();
	sequence->loadSequence(folder);
	sequence->preloadAllFrames();

	sequences[name] = sequence;
	return true;
 }
 */

template<typename T>
bool ofxAssetCache::exists(T& container, const string &name) {
	return container.find(name) != container.end();
}
