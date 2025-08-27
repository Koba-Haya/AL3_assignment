#define NOMINMAX 
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
	float param = std::sin((2.0f * std::numbers::pi_v<float>)*(walkTimer_ / kWalkMotionTime));
	float degree = kWalkMotionAngleStart + kWalkMotionAngleEnd * (param + 1.0f) / 2.0f;
	float angleRad = degree * (std::numbers::pi_v<float> / 180.0f);

	// 最後に AABB を更新
	const Vector3& p = worldTransform_.translation_;
	float half = kWidth * 0.5f;
	aabb_.min = {p.x - half, p.y - kHeight * 0.5f, p.z - half};
	aabb_.max = {p.x + half, p.y + kHeight * 0.5f, p.z + half};

	// 被弾フラッシュ減衰
	if (hurtFlashT_ > 0.0f)
		hurtFlashT_ = std::max(0.0f, hurtFlashT_ - 1.0f / 60.0f);

	// 死亡演出中は軽く沈める/縮小など入れてもOK
	if (isDead_) {
		deathT_ += 1.0f / 60.0f;
		// 例：少し沈む
		worldTransform_.translation_.y -= 0.02f;
	}

	// Z軸に対して角度反映（例: 足を振るなどに応用できる）
	worldTransform_.rotation_.z = angleRad;

	// 移動
	worldTransform_.translation_.x += velocity_.x;

	// 行列更新
	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Enemy::UpdateFreeze() {
	// 歩行タイマーや位置は進めない
	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Enemy::Draw() { model_->Draw(worldTransform_, *camera_); }

AABB Enemy::GetAABB() {
	Vector3 pos = {worldTransform_.matWorld_.m[3][0], worldTransform_.matWorld_.m[3][1], worldTransform_.matWorld_.m[3][2]}; // ワールド行列から座標取得
	AABB aabb;
	aabb.min = {pos.x - kWidth / 2.0f, pos.y - kHeight / 2.0f, pos.z};
	aabb.max = {pos.x + kWidth / 2.0f, pos.y + kHeight / 2.0f, pos.z};
	return aabb;
}

void Enemy::OnCollision(const Player* player) { (void)player; }

void Enemy::TakeDamage(int amount) {
	if (isDead_)
		return;
	hp_ -= amount;
	hurtFlashT_ = 0.15f; // 短い点滅

	if (hp_ <= 0) {
		isDead_ = true;
		// ここでは「消滅は GameScene 側」で行う
	}
}

void Enemy::ApplyKnockback(const Vector3& dir, float power) {
	Vector3 nd = Normalize(dir);                    // ← Normalize() は Method.h で宣言済み
	worldTransform_.translation_.x += nd.x * power; // ← 成分ごとに加算
	worldTransform_.translation_.y += nd.y * power;
	worldTransform_.translation_.z += nd.z * power;
}