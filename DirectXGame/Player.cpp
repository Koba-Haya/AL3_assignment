#include "Player.h"

void Player::Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera) {
	// NULLポインタチェック
	assert(model);

	// 引数として受け取ったデータをメンバ変数に記録
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	// ワールド変換の初期化
	worldTransform_.Initialize();
}

void Player::Update() { WorldTransformUpdate(worldTransform_); }

void Player::Draw() {
	// 3Dモデルを描画
	model_->Draw(worldTransform_, *camera_, textureHandle_);
}
