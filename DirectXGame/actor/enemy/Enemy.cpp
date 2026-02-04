#define NOMINMAX
#include "Enemy.h"
#include "MapChipField.h"
#include <algorithm>
#include <numbers>

using namespace KamataEngine;

void Enemy::Initialize(Model* model, Camera* camera, const Vector3& position) {
	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	velocity_ = {-kWalkSpeed, 0.0f, 0.0f};
	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;

	walkTimer_ = 0.0f;
	onGround_ = false;
}

void Enemy::Update() {
	const float dt = 1.0f / 60.0f;

	walkTimer_ += dt;
	if (walkTimer_ >= kWalkMotionTime) {
		walkTimer_ -= kWalkMotionTime;
	}

	float param = std::sin((2.0f * std::numbers::pi_v<float>) * (walkTimer_ / kWalkMotionTime));
	float degree = kWalkMotionAngleStart + (kWalkMotionAngleEnd - kWalkMotionAngleStart) * (param + 1.0f) * 0.5f;
	float angleRad = degree * (std::numbers::pi_v<float> / 180.0f);

	if (hurtFlashT_ > 0.0f) {
		hurtFlashT_ = std::max(0.0f, hurtFlashT_ - dt);
	}

	// ---- 死亡中も床判定（マップ衝突）を残す ----
	if (isDead_) {
		deathT_ += dt;

		velocity_.x = 0.0f;

		MoveWithMapCollision_();

		// 倒れる
		const float fallT = std::clamp(deathT_ / kDeathFallTimeSec, 0.0f, 1.0f);
		worldTransform_.rotation_.z = std::lerp(0.0f, -1.4f, fallT);

		// 縮小（倒れ切った後 → hold後）
		const float shrinkStart = kDeathFallTimeSec + kDeathHoldTimeSec;
		if (deathT_ >= shrinkStart) {
			const float sT = std::clamp((deathT_ - shrinkStart) / kDeathShrinkTimeSec, 0.0f, 1.0f);
			const float s = std::lerp(1.0f, 0.0f, sT);
			worldTransform_.scale_ = {s, s, s};
		} else {
			worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
		}

		// AABB更新（縮小したら当たり判定も小さくする）
		const Vector3& p = worldTransform_.translation_;
		const float half = (kWidth * 0.5f) * worldTransform_.scale_.x;
		const float halfH = (kHeight * 0.5f) * worldTransform_.scale_.y;

		aabb_.min = {p.x - half, p.y - halfH, p.z - half};
		aabb_.max = {p.x + half, p.y + halfH, p.z + half};

		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();

		worldTransform_.translation_.x += externalMove_.x;
		worldTransform_.translation_.y += externalMove_.y;
		worldTransform_.translation_.z += externalMove_.z;
		externalMove_ = {0.0f, 0.0f, 0.0f};
		return;
	}

	// ---- 生存時の通常移動 ----
	MoveWithMapCollision_();

	worldTransform_.rotation_.z = angleRad;

	const Vector3& p = worldTransform_.translation_;
	const float half = kWidth * 0.5f;
	aabb_.min = {p.x - half, p.y - kHeight * 0.5f, p.z - half};
	aabb_.max = {p.x + half, p.y + kHeight * 0.5f, p.z + half};

	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();

	worldTransform_.translation_.x += externalMove_.x;
	worldTransform_.translation_.y += externalMove_.y;
	worldTransform_.translation_.z += externalMove_.z;
	externalMove_ = {0.0f, 0.0f, 0.0f};
}

void Enemy::MoveWithMapCollision_() {
	if (!mapChipField_) {
		worldTransform_.translation_.x += velocity_.x;
		return;
	}

	// 重力
	if (!onGround_) {
		velocity_.y -= kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}

	// 崖判定で反転（onGround_ のときだけ）
	if (onGround_ && WillFallFromEdge_(velocity_.x)) {
		velocity_.x = -velocity_.x;
		worldTransform_.rotation_.y = (velocity_.x < 0.0f) ? -std::numbers::pi_v<float> / 2.0f : +std::numbers::pi_v<float> / 2.0f;
	}

	// X軸だけ解決
	CollisionInfo xInfo{};
	xInfo.moveAmount = {velocity_.x, 0.0f, 0.0f};
	MapCollisionRight_(xInfo);
	MapCollisionLeft_(xInfo);
	ApplyCollisionMove_(xInfo);

	// 壁に当たったら反転
	if (xInfo.onWallCollision) {
		velocity_.x = -velocity_.x;
		worldTransform_.rotation_.y = (velocity_.x < 0.0f) ? -std::numbers::pi_v<float> / 2.0f : +std::numbers::pi_v<float> / 2.0f;
	}

	// Y軸だけ解決
	CollisionInfo yInfo{};
	yInfo.moveAmount = {0.0f, velocity_.y, 0.0f};
	MapCollisionUp_(yInfo);
	MapCollisionDown_(yInfo);
	ApplyCollisionMove_(yInfo);

	// 接地/天井処理
	if (yInfo.onGroundCollision) {
		onGround_ = true;
		velocity_.y = 0.0f;
	} else {
		onGround_ = false;
	}

	if (yInfo.onCeilingCollision) {
		velocity_.y = 0.0f;
	}
}

bool Enemy::WillFallFromEdge_(float nextMoveX) const {
	if (!mapChipField_) {
		return false;
	}
	if (nextMoveX == 0.0f) {
		return false;
	}
	if (!onGround_) {
		return false;
	}

	const float dir = (nextMoveX > 0.0f) ? 1.0f : -1.0f;

	const Vector3 pos = worldTransform_.translation_;
	const float footY = pos.y - kHeight * 0.5f;

	Vector3 probe{};
	probe.x = pos.x + nextMoveX + dir * (kWidth * 0.5f + 0.05f);
	probe.y = footY - 0.02f;
	probe.z = pos.z;

	IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(probe);
	MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
	return !mapChipField_->IsSolid(type);
}

void Enemy::SetMapChipField(MapChipField* mapChipField) {
	mapChipField_ = mapChipField;
	if (mapChipField_) {
		SnapToGround_();
	}
}

void Enemy::SnapToGround_() {
	// 足元を少し下に見て、床があるなら「床 top + 半身」にスナップ
	const float eps = 0.001f;
	const float inset = 0.002f;
	const float probeDepth = 0.3f;

	const Vector3 pos = worldTransform_.translation_;
	const float bottom = pos.y - kHeight * 0.5f;

	Vector3 lb = {pos.x - kWidth * 0.5f + inset, bottom - probeDepth, pos.z};
	Vector3 rb = {pos.x + kWidth * 0.5f - inset, bottom - probeDepth, pos.z};

	float bestTop = -FLT_MAX;
	bool hit = false;

	auto test = [&](const Vector3& p) {
		IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(p);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
		if (!mapChipField_->IsSolid(type)) {
			return;
		}
		Rect r = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
		bestTop = std::max(bestTop, r.top);
		hit = true;
	};

	test(lb);
	test(rb);

	if (!hit) {
		return;
	}

	// bottom を床 top に合わせる
	worldTransform_.translation_.y = bestTop + (kHeight * 0.5f) + eps;
	onGround_ = true;
	velocity_.y = 0.0f;
}

void Enemy::MapCollisionUp_(CollisionInfo& info) {
	if (info.moveAmount.y <= 0.0f) {
		return;
	}
	const float eps = 0.001f;
	const float inset = 0.002f;

	const Vector3 cur = worldTransform_.translation_;
	const float currentTop = cur.y + kHeight * 0.5f;

	Vector3 moved = {cur.x + info.moveAmount.x, cur.y + info.moveAmount.y, cur.z};

	Vector3 lt = {moved.x - kWidth * 0.5f + inset, moved.y + kHeight * 0.5f - inset, moved.z};
	Vector3 rt = {moved.x + kWidth * 0.5f - inset, moved.y + kHeight * 0.5f - inset, moved.z};

	bool hitAny = false;
	float allowedDy = info.moveAmount.y;

	auto testPoint = [&](const Vector3& p) {
		IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(p);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
		if (!mapChipField_->IsSolid(type)) {
			return;
		}
		Rect r = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
		float cand = (r.bottom - eps) - currentTop;
		allowedDy = std::min(allowedDy, cand);
		hitAny = true;
	};

	testPoint(lt);
	testPoint(rt);

	if (hitAny && allowedDy < info.moveAmount.y) {
		info.moveAmount.y = std::max(0.0f, allowedDy);
		info.onCeilingCollision = true;
	}
}

void Enemy::MapCollisionDown_(CollisionInfo& info) {
	// 0も見る：地面の上で微妙に沈んでる/浮いてるのを安定化させるため
	if (info.moveAmount.y > 0.0f) {
		return;
	}
	const float eps = 0.001f;
	const float inset = 0.002f;

	const Vector3 cur = worldTransform_.translation_;
	const float currentBottom = cur.y - kHeight * 0.5f;

	// X は移動後、Y は「今の足元」を基準に床を拾う（安定）
	const float nextX = cur.x + info.moveAmount.x;

	Vector3 lb = {nextX - kWidth * 0.5f + inset, currentBottom - 0.05f, cur.z};
	Vector3 rb = {nextX + kWidth * 0.5f - inset, currentBottom - 0.05f, cur.z};

	bool hitAny = false;
	float bestTop = -FLT_MAX;

	auto testPoint = [&](const Vector3& p) {
		IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(p);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
		if (!mapChipField_->IsSolid(type)) {
			return;
		}
		Rect r = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
		bestTop = std::max(bestTop, r.top);
		hitAny = true;
	};

	testPoint(lb);
	testPoint(rb);

	if (!hitAny) {
		return;
	}

	// bottom を床 top に合わせる（沈んでれば +、落下中なら落下量を減らす）
	const float snapDy = (bestTop + eps) - currentBottom;

	// もともとの移動量より「上方向に強い」補正が必要なら採用（めり込み解消）
	// もともと落下中でも、床が近いなら落下量を縮める
	if (snapDy > info.moveAmount.y) {
		info.moveAmount.y = snapDy;
		info.onGroundCollision = true;
	}
}

void Enemy::MapCollisionRight_(CollisionInfo& info) {
	if (info.moveAmount.x <= 0.0f) {
		return;
	}
	const float eps = 0.001f;
	const float inset = 0.002f;

	const Vector3 cur = worldTransform_.translation_;
	const float currentRight = cur.x + kWidth * 0.5f;

	Vector3 moved = {cur.x + info.moveAmount.x, cur.y + info.moveAmount.y, cur.z};

	Vector3 rt = {moved.x + kWidth * 0.5f - inset, moved.y + kHeight * 0.5f - inset, moved.z};
	Vector3 rb = {moved.x + kWidth * 0.5f - inset, moved.y - kHeight * 0.5f + inset, moved.z};

	bool hitAny = false;
	float allowedDx = info.moveAmount.x;

	auto testPoint = [&](const Vector3& p) {
		IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(p);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
		if (!mapChipField_->IsSolid(type)) {
			return;
		}
		Rect r = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
		float cand = (r.left - eps) - currentRight;

		// “めり込み解消”のため、cand が負でも採用できるようにする
		allowedDx = std::min(allowedDx, cand);
		hitAny = true;
	};

	testPoint(rt);
	testPoint(rb);

	if (hitAny && allowedDx != info.moveAmount.x) {
		info.moveAmount.x = allowedDx; // 0クランプしない
		info.onWallCollision = true;
		info.wallNormalX = -1;
	}
}

void Enemy::MapCollisionLeft_(CollisionInfo& info) {
	if (info.moveAmount.x >= 0.0f) {
		return;
	}
	const float eps = 0.001f;
	const float inset = 0.002f;

	const Vector3 cur = worldTransform_.translation_;
	const float currentLeft = cur.x - kWidth * 0.5f;

	Vector3 moved = {cur.x + info.moveAmount.x, cur.y + info.moveAmount.y, cur.z};

	Vector3 lt = {moved.x - kWidth * 0.5f + inset, moved.y + kHeight * 0.5f - inset, moved.z};
	Vector3 lb = {moved.x - kWidth * 0.5f + inset, moved.y - kHeight * 0.5f + inset, moved.z};

	bool hitAny = false;
	float allowedDx = info.moveAmount.x;

	auto testPoint = [&](const Vector3& p) {
		IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(p);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
		if (!mapChipField_->IsSolid(type)) {
			return;
		}
		Rect r = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
		float cand = (r.right + eps) - currentLeft;

		// めり込みなら cand が正になって押し戻し（右へ）が必要になることがある
		allowedDx = std::max(allowedDx, cand);
		hitAny = true;
	};

	testPoint(lt);
	testPoint(lb);

	if (hitAny && allowedDx != info.moveAmount.x) {
		info.moveAmount.x = allowedDx; // 0クランプしない
		info.onWallCollision = true;
		info.wallNormalX = +1;
	}
}

void Enemy::ApplyCollisionMove_(const CollisionInfo& info) {
	worldTransform_.translation_.x += info.moveAmount.x;
	worldTransform_.translation_.y += info.moveAmount.y;
	worldTransform_.translation_.z += info.moveAmount.z;
}

void Enemy::HandleGroundCeiling_(const CollisionInfo& info) {
	if (info.onGroundCollision) {
		onGround_ = true;
		velocity_.y = 0.0f;
	} else {
		onGround_ = false;
	}

	if (info.onCeilingCollision) {
		velocity_.y = 0.0f;
	}
}

void Enemy::HandleWall_(const CollisionInfo& info) {
	if (!info.onWallCollision) {
		return;
	}
	velocity_.x = -velocity_.x;
	worldTransform_.rotation_.y = (velocity_.x < 0.0f) ? -std::numbers::pi_v<float> / 2.0f : +std::numbers::pi_v<float> / 2.0f;
}

void Enemy::UpdateFreeze() {
	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Enemy::Draw() {
	model_->Draw(worldTransform_, *camera_);
}

AABB Enemy::GetAABB() {
	return aabb_;
}

void Enemy::OnCollision(const Player* player) {
	(void)player;
}

void Enemy::TakeDamage(int amount) {
	if (isDead_) {
		return;
	}
	hp_ -= amount;
	hurtFlashT_ = 0.15f;

	if (hp_ <= 0) {
		isDead_ = true;
	}
}

void Enemy::ApplyKnockback(const Vector3& dir, float power) {
	Vector3 nd = Normalize(dir);
	worldTransform_.translation_.x += nd.x * power;
	worldTransform_.translation_.y += nd.y * power;
	worldTransform_.translation_.z += nd.z * power;
}

void Enemy::AddExternalMove(const Vector3& delta) {
	externalMove_.x += delta.x;
	externalMove_.y += delta.y;
	externalMove_.z += delta.z;
}

bool Enemy::IsDeathEffectFinished() const {
	if (!isDead_) {
		return false;
	}
	const float endT = kDeathFallTimeSec + kDeathHoldTimeSec + kDeathShrinkTimeSec;
	return deathT_ >= endT;
}
