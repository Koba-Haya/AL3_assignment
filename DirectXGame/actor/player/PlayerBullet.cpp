#include "PlayerBullet.h"
#include <algorithm>

using namespace KamataEngine;

void PlayerBullet::Initialize(Model* model, Camera* camera, const Vector3& pos, const Vector3& vel) {
	model_ = model;
	camera_ = camera;
	velocity_ = vel;

	worldTransform_.Initialize();
	worldTransform_.translation_ = pos;

	active_ = true;
	lifeSec_ = 0.0f;

	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void PlayerBullet::Update() {
	if (!active_) {
		return;
	}

	const float dt = 1.0f / 60.0f;

	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;

	lifeSec_ += dt;
	if (lifeSec_ >= kLifeTimeSec) {
		active_ = false;
		return;
	}

	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void PlayerBullet::Draw() {
	if (!active_ || !model_) {
		return;
	}
	model_->Draw(worldTransform_, *camera_);
}

AABB PlayerBullet::GetAABB() const {
	const Vector3& p = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {p.x - kRadius, p.y - kRadius, p.z - kRadius};
	aabb.max = {p.x + kRadius, p.y + kRadius, p.z + kRadius};
	return aabb;
}