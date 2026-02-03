#include "Goal.h"
#include <algorithm>
#include <numbers>

void Goal::Initialize(Model* model, const Vector3& pos) {
	model_ = model;
	worldTransform_.Initialize();
	worldTransform_.translation_ = pos;
	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;
	RebuildAABB_();
	active_ = true;
}

void Goal::Update(/* float dt */) {
	// お好み演出（例: くるっと回す）
	// wt_.rotation_.y += dt * 0.8f;
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Goal::Draw(Camera& camera) {
	if (!active_ || !model_)
		return;
	model_->Draw(worldTransform_, camera);
}
