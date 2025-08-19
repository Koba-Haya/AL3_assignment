#include "Skydome.h"

using namespace KamataEngine;

void Skydome::Initialize() {
	// ワールド変換の初期化
	worldTransform_.Initialize();
	model_ = Model::CreateFromOBJ("sky_sphere", true); // ここでモデルを読み込む
}

void Skydome::Update() {}

void Skydome::Draw(KamataEngine::Camera *camera_) { model_->Draw(worldTransform_, *camera_); }
