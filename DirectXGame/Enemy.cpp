#include "Enemy.h"
#include <algorithm>
#include <numbers>

using namespace KamataEngine;

void Enemy::Initialize(Model* model, Camera* camera, const Vector3& position) {
	model_ = model;
	camera_ = camera;

	// ワールド変換の初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;

	// 移動速度の初期化（左方向）
	velocity_ = {-kWalkSpeed, 0.0f, 0.0f};

	// タイマー初期化
	walkTimer_ = 0.0f;
}

void Enemy::Update() {
	// 時間経過
	walkTimer_ += 1.0f / 60.0f;

	// t を [0,1] にループ
	float t = walkTimer_ / kWalkMotionTime;
	if (t >= 1.0f) {
		t -= 1.0f;
		walkTimer_ = 0.0f;
	}

	// 角度補間 [degree → radian変換]
	float param = std::sin(2.0f * std::numbers::pi_v<float> * walkTimer_ / kWalkMotionTime);
	float degree = kWalkMotionAngleStart + kWalkMotionAngleEnd * (param + 1.0f) / 2.0f;
	float angleRad = degree * (std::numbers::pi_v<float> / 180.0f);

	// Z軸に対して角度反映（例: 足を振るなどに応用できる）
	worldTransform_.rotation_.z = angleRad;

	// 移動
	worldTransform_.translation_.x += velocity_.x;

	// 行列更新
	WorldTransformUpdate(worldTransform_);
}

void Enemy::Draw() { model_->Draw(worldTransform_, *camera_); }