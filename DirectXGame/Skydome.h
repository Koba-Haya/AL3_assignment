#pragma once
#include "KamataEngine.h"

using namespace KamataEngine;

class Skydome {
public:
	void Initialize();
	void Update();
	void Draw(KamataEngine::Camera *camera_);

	private:
		// ワールド変換データ
	    KamataEngine::WorldTransform worldTransform_;
		// モデル
	    Model* model_ = nullptr;
};
