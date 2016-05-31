//
//  ofxAssetCache.h
//  ofxAssetCache
//
//  Created by Matt Felsen on 12/7/15.
//
//

#pragma once

#include "ofMain.h"
//#include "ofxImageSequence.h"
//#include "ofxTurboJpeg.h"

// helper for shorter syntax
class ofxAssetCache;
typedef ofxAssetCache Assets;

class ofxAssetCache : public ofThread {
public:
	ofxAssetCache();
	~ofxAssetCache();

	void update(ofEventArgs& args);

	// Accessing assets (as shared_ptrs)
	// Use these to access shared assets after they've been added
	shared_ptr<ofImage> image(const string& name);
	shared_ptr<ofTexture> texture(const string& name);
	shared_ptr<ofVideoPlayer> video(const string& name);
	shared_ptr<ofShader> shader(const string& name);
//	shared_ptr<ofxImageSequence> sequence(const string& name);

	// Accessing assets (as raw pointers)
	// Use this for somewhat more semantic access to the raw pointer
	// ie this:      image->setImagePointer(Assets::get().image("name").get())
	// becomes this: image->setImagePointer(Assets::get().imagePointer("name"))
	ofImage* imagePointer(const string& name);
	ofTexture* texturePointer(const string& name);
	ofVideoPlayer* videoPointer(const string& name);

	// Adding assets
	// Use these to load new assets. If already loaded, asset will be skipped

	// Images and textures can be loaded by passing in a filename or an existing
	// ofPixels object. These methods are blocking
	// You can use the async version which loads data from the filename on a
	// thread and then uploads to the gpu on the main thread
	bool addImage(const string& name, const string& filename = "", bool mipmaps = false);
	bool addImage(const string& name, ofPixels& pix, bool mipmaps = false);
	void addImageAsync(const string& name, const string& filename = "", bool mipmaps = false);

	bool addTexture(const string& name, const string& filename = "", bool mipmaps = false);
	bool addTexture(const string& name, ofPixels& pix, bool mipmaps = false);
	void addTextureAsync(const string& name, const string& filename = "", bool mipmaps = false);

	bool addVideo(const string& name, const string& filename = "");

	bool addShader(const string& name, const string& filename = "");

//	bool addSequence(const string& name, const string& folderName = "");

	void threadedFunction();

	// Singleton
	static ofxAssetCache& get() {
		static ofxAssetCache instance;
		return instance;
	}

protected:
	enum QueuedItemType {
		IMAGE,
		TEXTURE
	};

	struct QueuedItem {
		QueuedItemType type;
		string name;
		string filename;
		bool mipmaps;
		ofPixels pixels;
	};

	template<typename T>
	bool exists(T& container, const string &name);

	bool loadToPixels(const string& file, ofPixels& pix);

	unordered_map<string, shared_ptr<ofImage>> images;
	unordered_map<string, shared_ptr<ofTexture>> textures;
	unordered_map<string, shared_ptr<ofVideoPlayer>> videos;
	unordered_map<string, shared_ptr<ofShader>> shaders;
//	unordered_map<string, shared_ptr<ofxImageSequence>> sequences;

	// empty content to return instead of null pointers
	shared_ptr<ofImage> emptyImage;
	shared_ptr<ofTexture> emptyTexture;
	shared_ptr<ofVideoPlayer> emptyVideo;

	deque<QueuedItem> cpuQueue;
	deque<QueuedItem> gpuQueue;

private:
//	ofxTurboJpeg turbo;

	ofxAssetCache(ofxAssetCache const&);
	void operator=(ofxAssetCache const&);
};
