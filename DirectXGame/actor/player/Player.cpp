#define NOMINMAX
#include "Player.h"
#include "MapChipField.h"
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

void Player::Initialize(Model* model, Camera* camera, const Vector3& position) {
	assert(model);
	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
}

float Player::Dot3(const Vector3& a, const Vector3& b) const { return a.x * b.x + a.y * b.y + a.z * b.z; }

void Player::ToggleWire() {
	if (state_ == ActionState::Dead) {
		return;
	}

	if (state_ == ActionState::WireShoot || state_ == ActionState::WireHang) {
		EndWire();
		return;
	}

	if (!CanUseWire()) {
		return;
	}

	BeginWireShoot();
}

void Player::BeginWireShoot() {
	++wiresUsed_;

	const float dirX = (lrDirection_ == LRDirection::kRight) ? 1.0f : -1.0f;

	// ★斜め上発射（正規化）
	wireShootDir_ = {dirX, wire_.shootUpFactor, 0.0f};
	wireShootDir_ = Normalize(wireShootDir_);

	wireTip_ = worldTransform_.translation_;
	wireTraveled_ = 0.0f;

	state_ = ActionState::WireShoot;
}

void Player::EndWire() {
	if (state_ != ActionState::WireShoot && state_ != ActionState::WireHang) {
		return;
	}

	state_ = ActionState::Move;
	wireEndedThisFrame_ = true;
}

Vector3 Player::GetWireVisualTarget() const {
	if (state_ == ActionState::WireHang) {
		return wireAnchor_;
	}
	if (state_ == ActionState::WireShoot) {
		return wireTip_;
	}
	return worldTransform_.translation_;
}

bool Player::TryAttachAtTip(Vector3& outAnchor) const {
	if (!mapChipField_) {
		return false;
	}

	IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(wireTip_);
	MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);

	// ★ダメージブロックにも刺さる
	if (type != MapChipType::kBlock && type != MapChipType::kDamage) {
		return false;
	}

	Rect r = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);

	// ★斜め上対応：どの面に刺さったかを射出方向から決める（簡易）
	const float ax = std::fabs(wireShootDir_.x);
	const float ay = std::fabs(wireShootDir_.y);

	if (ax >= ay) {
		// 左右面
		if (wireShootDir_.x > 0.0f) {
			outAnchor = {r.left - wire_.attachInset, wireTip_.y, 0.0f};
		} else {
			outAnchor = {r.right + wire_.attachInset, wireTip_.y, 0.0f};
		}
		outAnchor.y = std::clamp(outAnchor.y, r.bottom + 0.02f, r.top - 0.02f);
	} else {
		// 上下面（今回の発射は上向きなので基本は下面に刺さる想定）
		if (wireShootDir_.y > 0.0f) {
			outAnchor = {wireTip_.x, r.bottom - wire_.attachInset, 0.0f};
		} else {
			outAnchor = {wireTip_.x, r.top + wire_.attachInset, 0.0f};
		}
		outAnchor.x = std::clamp(outAnchor.x, r.left + 0.02f, r.right - 0.02f);
	}

	return true;
}

void Player::ApplyRopeConstraint() {
	Vector3 pos = worldTransform_.translation_;
	Vector3 diff = {pos.x - wireAnchor_.x, pos.y - wireAnchor_.y, pos.z - wireAnchor_.z};

	float dist = Length(diff);
	if (dist < 1e-6f) {
		return;
	}

	Vector3 n = {diff.x / dist, diff.y / dist, diff.z / dist};

	worldTransform_.translation_ = {wireAnchor_.x + n.x * wireLength_, wireAnchor_.y + n.y * wireLength_, wireAnchor_.z + n.z * wireLength_};

	const float vn = Dot3(velocity_, n);
	velocity_.x -= n.x * vn;
	velocity_.y -= n.y * vn;
	velocity_.z -= n.z * vn;
}

void Player::UpdateWireShoot() {
	const float step = wire_.shootSpeed;

	// ★斜め上へ進める
	wireTip_.x += wireShootDir_.x * step;
	wireTip_.y += wireShootDir_.y * step;
	wireTip_.z += wireShootDir_.z * step;

	wireTraveled_ += step;

	// ★追加：発射中のロープ長は「プレイヤー→先端」の距離で更新しておく
	{
		const Vector3 p = worldTransform_.translation_;
		const Vector3 toTip = {wireTip_.x - p.x, wireTip_.y - p.y, wireTip_.z - p.z};
		wireLength_ = std::min(wire_.maxDistance, Length(toTip));
		wireLength_ = std::max(wireLength_, wire_.minLength);
	}

	if (wireTraveled_ >= wire_.maxDistance) {
		EndWire();
		return;
	}

	Vector3 anchor{};
	if (TryAttachAtTip(anchor)) {
		wireAnchor_ = anchor;

		// ★刺さった瞬間：今のロープ長(=先端まで)を保持しつつ、少し短い目標長へ巻き取る
		wireReelTargetLength_ = std::max(wire_.minLength, wireLength_ - wire_.reelInShorten);

		state_ = ActionState::WireHang;
	}
}

void Player::UpdateWireHang() {
	// ★追加：巻き取り（wireLength_ を目標まで短くする）
	if (wireLength_ > wireReelTargetLength_) {
		wireLength_ = std::max(wireReelTargetLength_, wireLength_ - wire_.reelInSpeed);
	}

	auto* input = Input::GetInstance();

	velocity_.y -= kGravityAcceleration;
	velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);

	if (input->PushKey(DIK_D)) {
		velocity_.x += wire_.swingInputAccel;
	}
	if (input->PushKey(DIK_A)) {
		velocity_.x -= wire_.swingInputAccel;
	}

	if (wire_.airDamping > 0.0f) {
		velocity_.x *= (1.0f - wire_.airDamping);
		velocity_.y *= (1.0f - wire_.airDamping);
	}

	CollisionMapInfo wallInfo{};
	wallInfo.moveAmount_ = {velocity_.x, 0.0f, 0.0f};
	MapCollisionDetectionRight(wallInfo);
	MapCollisionDetectionLeft(wallInfo);
	ApplyCollisionMove(wallInfo);

	CollisionMapInfo verticalInfo{};
	verticalInfo.moveAmount_ = {0.0f, velocity_.y, 0.0f};
	MapCollisionDetectionUp(verticalInfo);
	MapCollisionDetectionDown(verticalInfo);
	ApplyCollisionMove(verticalInfo);

	HandleCeilingCollision(verticalInfo);
	HandleGroundCollision(verticalInfo);
	HandleWallCollision(wallInfo);

	if (wallInfo.onWallCollision_) {
		velocity_.x = 0.0f;
	}
	if (verticalInfo.onGroundCollision_ || verticalInfo.onCeilingCollision_) {
		velocity_.y = 0.0f;
	}

	// ロープ拘束（巻き取り後の wireLength_ が効いて引っ張られる）
	ApplyRopeConstraint();
}

void Player::StartWire(const Vector3& targetWorldPos) { (void)targetWorldPos; }

void Player::CancelWireKeepInertia() {}

bool Player::IsOnLadder_() const {
	if (!mapChipField_) {
		return false;
	}

	// プレイヤー中心で判定（必要ならAABBの中心/足元に変更）
	IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_);
	MapChipType t = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
	return t == MapChipType::kLadder;
}

void Player::Update() {
	float dt = 1.0f / 60.0f;

	UpdateReload();

	if (wallDetachLeft_ > 0.0f) {
		wallDetachLeft_ = std::max(0.0f, wallDetachLeft_ - dt);
		if (wallDetachLeft_ == 0.0f) {
			wallDetachDirX_ = 0;
		}
	}

	switch (state_) {
	case ActionState::Move: {
		// ★梯子にいる＆W押下で梯子モードへ
		if (IsOnLadder_() && Input::GetInstance()->PushKey(DIK_W)) {
			state_ = ActionState::Ladder;
			velocity_ = {0.0f, 0.0f, 0.0f};
			onGround_ = false;
			onWall_ = false;
		}

		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			jumpBufferLeft_ = jumpBufferTime_;
		}

		Move();

		CollisionMapInfo wallInfo{};
		wallInfo.moveAmount_ = {velocity_.x, 0.0f, 0.0f};
		MapCollisionDetectionRight(wallInfo);
		MapCollisionDetectionLeft(wallInfo);
		ApplyCollisionMove(wallInfo);

		CollisionMapInfo verticalInfo{};
		verticalInfo.moveAmount_ = {0.0f, velocity_.y, 0.0f};
		MapCollisionDetectionUp(verticalInfo);
		MapCollisionDetectionDown(verticalInfo);
		ApplyCollisionMove(verticalInfo);

		HandleCeilingCollision(verticalInfo);
		HandleGroundCollision(verticalInfo);
		HandleWallCollision(wallInfo);

		if (onGround_) {
			coyoteLeft_ = coyoteTime_;
		} else {
			coyoteLeft_ = std::max(0.0f, coyoteLeft_ - dt);
		}

		bool wantJump = (jumpBufferLeft_ > 0.0f);
		bool canWallJump = (!onGround_) && (wallJumpsUsed_ < maxWallJumps_) && (wallDetachLeft_ <= 0.0f);

		if (wantJump && canWallJump && wallInfo.onWallCollision_ && wallInfo.wallNormalX_ != 0) {
			WallJump(wallInfo.wallNormalX_);
			jumpBufferLeft_ = 0.0f;
			coyoteLeft_ = 0.0f;
		} else {
			bool onWall = wallInfo.onWallCollision_;
			bool canGroundJump = (onGround_ || coyoteLeft_ > 0.0f);
			bool canAirJump = (!onGround_ && !onWall && (jumpsUsed_ < maxJumps_));

			if (wantJump && !onWall && (canGroundJump || canAirJump)) {
				velocity_.y = kJumpAcceleration;
				onGround_ = false;
				++jumpsUsed_;
				jumpBufferLeft_ = 0.0f;
				coyoteLeft_ = 0.0f;
			}
		}

		jumpBufferLeft_ = std::max(0.0f, jumpBufferLeft_ - dt);

		// 回転
		if (turnTimer_ > 0.0f) {
			turnTimer_ -= dt;

			float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
			float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

			float t = 1.0f - std::clamp(turnTimer_ / kTimeTurn, 0.0f, 1.0f);
			float easedT = EaseInOut(t);
			worldTransform_.rotation_.y = std::lerp(turnFirstRotationY_, destinationRotationY, easedT);
		}

		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}

	case ActionState::Ladder: {
		// ★梯子から外れたら通常へ
		if (!IsOnLadder_()) {
			state_ = ActionState::Move;
			break;
		}

		auto* in = Input::GetInstance();

		float dy = 0.0f;
		if (in->PushKey(DIK_W)) {
			dy += kLadderClimbSpeed;
		}
		if (in->PushKey(DIK_S)) {
			dy -= kLadderClimbSpeed;
		}

		// 梯子中は重力なし・横移動なし（必要ならA/Dで横移動も追加可）
		velocity_ = {0.0f, dy, 0.0f};

		// ★Y方向だけブロック衝突（天井/床は止まる）
		CollisionMapInfo verticalInfo{};
		verticalInfo.moveAmount_ = {0.0f, velocity_.y, 0.0f};
		MapCollisionDetectionUp(verticalInfo);
		MapCollisionDetectionDown(verticalInfo);
		ApplyCollisionMove(verticalInfo);

		HandleCeilingCollision(verticalInfo);
		HandleGroundCollision(verticalInfo);

		// 乗ってる間は「接地扱いにしたくない」なら上書き（必要に応じて）
		onGround_ = false;

		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}

	case ActionState::WireShoot: {
		UpdateWireShoot();
		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}

	case ActionState::WireHang: {
		UpdateWireHang();
		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}

	case ActionState::DashAttack: {
		UpdateDashAttack();
		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}

	case ActionState::Dead:
		break;
	}

	// ★外部移動（床など）を最後に1回だけ適用
	worldTransform_.translation_.x += externalMove_.x;
	worldTransform_.translation_.y += externalMove_.y;
	worldTransform_.translation_.z += externalMove_.z;
	externalMove_ = {0.0f, 0.0f, 0.0f};
}

void Player::UpdateFreeze() {
	// ★外部移動（床など）を適用
	worldTransform_.translation_.x += externalMove_.x;
	worldTransform_.translation_.y += externalMove_.y;
	worldTransform_.translation_.z += externalMove_.z;
	externalMove_ = {0.0f, 0.0f, 0.0f};

	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Player::Draw() { model_->Draw(worldTransform_, *camera_); }

void Player::Move() {

	// 左右入力（A/D）
	if (Input::GetInstance()->PushKey(DIK_D) || Input::GetInstance()->PushKey(DIK_A)) {

		Vector3 acceleration{};

		if (Input::GetInstance()->PushKey(DIK_D)) {
			if (velocity_.x < 0.0f) {
				velocity_.x *= (1.0f - kAttenuation);
			}
			acceleration.x += kAcceleration;

			if (lrDirection_ != LRDirection::kRight) {
				lrDirection_ = LRDirection::kRight;
				turnFirstRotationY_ = worldTransform_.rotation_.y;
				turnTimer_ = kTimeTurn;
			}
		} else if (Input::GetInstance()->PushKey(DIK_A)) {
			if (velocity_.x > 0.0f) {
				velocity_.x *= (1.0f - kAttenuation);
			}
			acceleration.x -= kAcceleration;

			if (lrDirection_ != LRDirection::kLeft) {
				lrDirection_ = LRDirection::kLeft;
				turnFirstRotationY_ = worldTransform_.rotation_.y;
				turnTimer_ = kTimeTurn;
			}
		}

		// 壁ジャンプ直後は壁方向への入力を無効化（地上でも安全のため残す）
		if (wallDetachLeft_ > 0.0f && wallDetachDirX_ != 0) {
			if (acceleration.x * static_cast<float>(wallDetachDirX_) < 0.0f) {
				acceleration.x = 0.0f;
			}
			if (velocity_.x * static_cast<float>(wallDetachDirX_) < 0.0f) {
				velocity_.x = 0.0f;
			}
		}

		velocity_.x += acceleration.x;

		float maxSpeedX = kRimitRunSpeed;
		if (wallDetachLeft_ > 0.0f) {
			maxSpeedX = std::max(kRimitRunSpeed, kWallJumpMaxHorizontalSpeed);
		}
		velocity_.x = std::clamp(velocity_.x, -maxSpeedX, maxSpeedX);

	} else {
		velocity_.x *= (1.0f - kAttenuation);
	}

	// 重力（Move中のみ）
	if (!onGround_) {
		velocity_.y -= kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
}

const KamataEngine::WorldTransform& Player::GetWorldTransform() const { return worldTransform_; }

void Player::SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

void Player::MapCollisionDetection(CollisionMapInfo& info) {
	MapCollisionDetectionUp(info);
	MapCollisionDetectionDown(info);
	MapCollisionDetectionRight(info);
	MapCollisionDetectionLeft(info);
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	static const Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0},
        {-kWidth / 2.0f, -kHeight / 2.0f, 0},
        {+kWidth / 2.0f, +kHeight / 2.0f, 0},
        {-kWidth / 2.0f, +kHeight / 2.0f, 0}
    };
	return {center.x + offsetTable[static_cast<uint32_t>(corner)].x, center.y + offsetTable[static_cast<uint32_t>(corner)].y, center.z + offsetTable[static_cast<uint32_t>(corner)].z};
}

// --- ここから下の MapCollisionDetectionUp/Down/Right/Left, ApplyCollisionMove, Handle*** は、
// --- あなたが貼ってくれたものをそのまま使う（変更なし）ので、ここでは省略しないで丸ごと貼る

void Player::MapCollisionDetectionUp(CollisionMapInfo& info) {
	// 上方向に動いているときだけ天井判定
	if (info.moveAmount_.y <= 0.0f) {
		return;
	}

	const float eps = 0.001f;
	const float sampleInset = 0.002f; // サンプル点を少し内側へ（境界ブレ対策）

	const Vector3 cur = worldTransform_.translation_;
	const float currentTop = cur.y + kHeight * 0.5f;

	// 移動後座標
	Vector3 moved = {cur.x + info.moveAmount_.x, cur.y + info.moveAmount_.y, cur.z + info.moveAmount_.z};

	// 上面2点（サンプルは“内側”へ寄せる：yは少し下へ）
	Vector3 lt = CornerPosition(moved, Corner::kLeftTop);
	Vector3 rt = CornerPosition(moved, Corner::kRightTop);
	lt.y -= sampleInset;
	rt.y -= sampleInset;

	bool hitAny = false;
	float allowedDy = info.moveAmount_.y;

	auto testPoint = [&](const Vector3& pIn) {
		Vector3 p = pIn;
		IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(p);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
		if (!mapChipField_->IsSolid(type)) {
			return;
		}
		Rect r = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);

		// 天井（ブロックの底）までの許容dy
		float cand = (r.bottom - eps) - currentTop;
		allowedDy = std::min(allowedDy, cand);
		hitAny = true;
	};

	testPoint(lt);
	testPoint(rt);

	if (hitAny && allowedDy < info.moveAmount_.y) {
		info.moveAmount_.y = std::max(0.0f, allowedDy);
		info.onCeilingCollision_ = true;
	}
}

void Player::MapCollisionDetectionDown(CollisionMapInfo& info) {
	// 下方向に動いているときだけ足元判定
	if (info.moveAmount_.y >= 0.0f) {
		return;
	}

	const float eps = 0.001f;
	const float sampleInset = 0.002f;

	const Vector3 cur = worldTransform_.translation_;
	const float currentBottom = cur.y - kHeight * 0.5f;

	Vector3 moved = {cur.x + info.moveAmount_.x, cur.y + info.moveAmount_.y, cur.z + info.moveAmount_.z};

	// 下面2点（サンプルは“内側”へ寄せる：yは少し上へ）
	Vector3 lb = CornerPosition(moved, Corner::kLeftBottom);
	Vector3 rb = CornerPosition(moved, Corner::kRightBottom);
	lb.y += sampleInset;
	rb.y += sampleInset;

	bool hitAny = false;
	float allowedDy = info.moveAmount_.y;

	auto testPoint = [&](const Vector3& pIn) {
		Vector3 p = pIn;
		IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(p);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
		if (!mapChipField_->IsSolid(type)) {
			return;
		}
		Rect r = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);

		// 地面（ブロックの天面）までの許容dy
		float cand = (r.top + eps) - currentBottom;
		allowedDy = std::max(allowedDy, cand);
		hitAny = true;
	};

	testPoint(lb);
	testPoint(rb);

	if (hitAny && allowedDy > info.moveAmount_.y) {
		info.moveAmount_.y = std::min(0.0f, allowedDy);
		info.onGroundCollision_ = true;
	}
}

void Player::MapCollisionDetectionRight(CollisionMapInfo& info) {
	// 右方向へ動いているときの壁判定
	if (info.moveAmount_.x <= 0.0f) {
		return;
	}

	const float eps = 0.001f;
	const float sampleInset = 0.002f;

	const Vector3 cur = worldTransform_.translation_;
	const float currentRight = cur.x + kWidth * 0.5f;

	// すでにめり込んでいる場合の押し出し（サンプル点は内側へ寄せる：xは少し左へ）
	{
		Vector3 curRB = CornerPosition(cur, Corner::kRightBottom);
		Vector3 curRT = CornerPosition(cur, Corner::kRightTop);
		curRB.x -= sampleInset;
		curRT.x -= sampleInset;

		Rect hitRect{};
		bool hitAny = false;

		auto testPoint = [&](const Vector3& pIn) {
			Vector3 p = pIn;
			IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(p);
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
			if (!mapChipField_->IsSolid(type)) {
				return;
			}
			hitRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
			hitAny = true;
		};

		testPoint(curRB);
		testPoint(curRT);

		if (hitAny) {
			float penetration = currentRight - hitRect.left;
			// 0に近い“誤検出”では押し出さない（ガクガク対策）
			if (penetration > 0.002f) {
				float pushOut = -(penetration + eps);
				info.moveAmount_.x = std::min(info.moveAmount_.x, pushOut);
				info.clampedX_ = true;
				info.onWallCollision_ = true;
				info.wallNormalX_ = -1;
				return;
			}
		}
	}

	// 移動後に右側2点がブロックへ入るか（サンプルは内側へ寄せる：xは少し左へ）
	Vector3 moved = {cur.x + info.moveAmount_.x, cur.y + info.moveAmount_.y, cur.z + info.moveAmount_.z};
	Vector3 rb = CornerPosition(moved, Corner::kRightBottom);
	Vector3 rt = CornerPosition(moved, Corner::kRightTop);
	rb.x -= sampleInset;
	rt.x -= sampleInset;

	bool hitAny = false;
	float allowedDx = info.moveAmount_.x;

	auto testPoint = [&](const Vector3& pIn) {
		Vector3 p = pIn;
		IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(p);
		if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) != MapChipType::kBlock) {
			return;
		}
		Rect r = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
		float cand = (r.left - eps) - currentRight;
		allowedDx = std::min(allowedDx, cand);
		hitAny = true;
	};

	testPoint(rb);
	testPoint(rt);

	if (hitAny && allowedDx < info.moveAmount_.x) {
		info.moveAmount_.x = std::max(0.0f, allowedDx);
		info.clampedX_ = true;
		info.onWallCollision_ = true;
		info.wallNormalX_ = -1;
	}
}

void Player::MapCollisionDetectionLeft(CollisionMapInfo& info) {
	// 左方向へ動いているときの壁判定
	if (info.moveAmount_.x >= 0.0f) {
		return;
	}

	const float eps = 0.001f;
	const float sampleInset = 0.002f;

	const Vector3 cur = worldTransform_.translation_;
	const float currentLeft = cur.x - kWidth * 0.5f;

	// すでにめり込んでいる場合の押し出し（サンプル点は内側へ寄せる：xは少し右へ）
	{
		Vector3 curLB = CornerPosition(cur, Corner::kLeftBottom);
		Vector3 curLT = CornerPosition(cur, Corner::kLeftTop);
		curLB.x += sampleInset;
		curLT.x += sampleInset;

		Rect hitRect{};
		bool hitAny = false;

		auto testPoint = [&](const Vector3& pIn) {
			Vector3 p = pIn;
			IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(p);
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
			if (!mapChipField_->IsSolid(type)) {
				return;
			}
			hitRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
			hitAny = true;
		};

		testPoint(curLB);
		testPoint(curLT);

		if (hitAny) {
			float penetration = hitRect.right - currentLeft;
			if (penetration > 0.002f) {
				float pushOut = +(penetration + eps);
				info.moveAmount_.x = std::max(info.moveAmount_.x, pushOut);
				info.clampedX_ = true;
				info.onWallCollision_ = true;
				info.wallNormalX_ = +1;
				return;
			}
		}
	}

	// 移動後に左側2点がブロックへ入るか（サンプルは内側へ寄せる：xは少し右へ）
	Vector3 moved = {cur.x + info.moveAmount_.x, cur.y + info.moveAmount_.y, cur.z + info.moveAmount_.z};
	Vector3 lb = CornerPosition(moved, Corner::kLeftBottom);
	Vector3 lt = CornerPosition(moved, Corner::kLeftTop);
	lb.x += sampleInset;
	lt.x += sampleInset;

	bool hitAny = false;
	float allowedDx = info.moveAmount_.x;

	auto testPoint = [&](const Vector3& pIn) {
		Vector3 p = pIn;
		IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(p);
		if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) != MapChipType::kBlock) {
			return;
		}
		Rect r = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
		float cand = (r.right + eps) - currentLeft;
		allowedDx = std::max(allowedDx, cand);
		hitAny = true;
	};

	testPoint(lb);
	testPoint(lt);

	if (hitAny && allowedDx > info.moveAmount_.x) {
		info.moveAmount_.x = std::min(0.0f, allowedDx);
		info.onWallCollision_ = true;
		info.clampedX_ = true;
		info.wallNormalX_ = +1;
	}
}

void Player::ApplyCollisionMove(const CollisionMapInfo& info) {
	worldTransform_.translation_.x += info.moveAmount_.x;
	worldTransform_.translation_.y += info.moveAmount_.y;
	worldTransform_.translation_.z += info.moveAmount_.z;
}

void Player::HandleCeilingCollision(const CollisionMapInfo& info) {
	if (info.onCeilingCollision_) {
		velocity_.y = 0.0f;
		jumpBufferLeft_ = 0.0f;
	}
}

void Player::HandleGroundCollision(const CollisionMapInfo& info) {
	if (info.onGroundCollision_) {
		velocity_.y = 0.0f;
		onGround_ = true;
		onWall_ = false;

		jumpsUsed_ = 0;
		wallJumpsUsed_ = 0;

		// ワイヤー回数もここでだけ回復
		wiresUsed_ = 0;

		wallDetachLeft_ = 0.0f;
		wallDetachDirX_ = 0;

		// ワイヤー終了直後に着地した場合、走りより速い横滑りを抑える
		if (wireEndedThisFrame_) {
			velocity_.x = std::clamp(velocity_.x, -kRimitRunSpeed, kRimitRunSpeed);
			wireEndedThisFrame_ = false;
		}
	} else {
		onGround_ = false;
	}
}

void Player::HandleWallCollision(const CollisionMapInfo& info) {
	if (wallDetachLeft_ > 0.0f) {
		onWall_ = false;
		return;
	}

	onWall_ = info.onWallCollision_;

	if (!info.onWallCollision_) {
		return;
	}

	if (info.clampedX_) {
		bool intoRightWall = (info.wallNormalX_ == -1 && velocity_.x > 0.0f);
		bool intoLeftWall = (info.wallNormalX_ == +1 && velocity_.x < 0.0f);
		if (intoRightWall || intoLeftWall) {
			velocity_.x = 0.0f;
		}
	}

	auto* input = Input::GetInstance();
	bool pushRight = input->PushKey(DIK_D);
	bool pushLeft = input->PushKey(DIK_A);

	bool pushingIntoWall = (info.wallNormalX_ == -1 && pushRight) || (info.wallNormalX_ == +1 && pushLeft);

	if (pushingIntoWall && velocity_.y < 0.0f) {
		velocity_.y = std::max(velocity_.y, -kWallSlideFallSpeed);
	}

	coyoteLeft_ = 0.0f;
	onGround_ = false;
}

Vector3 Player::GetWorldPosition() {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Player::GetAABB() {
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};
	return aabb;
}

void Player::OnCollision(const Enemy* enemy) {
	(void)enemy;
	isDead_ = true;
	velocity_.y = kJumpAcceleration;
	state_ = ActionState::Dead;
}

int Player::GetWireLeft() const { return std::max(0, maxWires_ - wiresUsed_); }

void Player::WallJump(int wallNormalX) {
	++wallJumpsUsed_;

	worldTransform_.translation_.x += static_cast<float>(wallNormalX) * kWallJumpSeparation;

	wallDetachLeft_ = kWallDetachTime;
	wallDetachDirX_ = wallNormalX;

	velocity_.y = kWallJumpVerticalSpeed;

	float baseVX = kWallJumpHorizontalSpeed;
	float minVX = kWallJumpMinDetachVX;
	float vx = std::max(baseVX, minVX) * static_cast<float>(wallNormalX);
	velocity_.x = vx;

	if (wallNormalX > 0) {
		lrDirection_ = LRDirection::kRight;
	} else if (wallNormalX < 0) {
		lrDirection_ = LRDirection::kLeft;
	}

	jumpsUsed_ = std::max(jumpsUsed_, 1);

	onGround_ = false;
	onWall_ = false;
	coyoteLeft_ = 0.0f;
	jumpBufferLeft_ = 0.0f;
}

// 末尾付近に追加
void Player::AddExternalMove(const Vector3& delta) {
	externalMove_.x += delta.x;
	externalMove_.y += delta.y;
	externalMove_.z += delta.z;
}

bool Player::CanAttack() const {
	if (state_ == ActionState::Dead) {
		return false;
	}
	if (state_ == ActionState::WireShoot || state_ == ActionState::WireHang) {
		return false;
	}
	if (state_ == ActionState::Ladder) {
		return false;
	}
	if (state_ == ActionState::DashAttack) {
		return false;
	}
	return true;
}

void Player::BeginDashAttack() {
	if (!CanAttack()) {
		return;
	}

	state_ = ActionState::DashAttack;
	dashAttackLeft_ = kDashAttackTime;

	const float dir = (lrDirection_ == LRDirection::kRight) ? 1.0f : -1.0f;
	velocity_.x = dir * kDashAttackSpeed;
	velocity_.y = 0.0f;
}

void Player::UpdateDashAttack() {
	const float dt = 1.0f / 60.0f;

	dashAttackLeft_ = std::max(0.0f, dashAttackLeft_ - dt);

	// 突進中は X のみ移動＋壁で止まる
	CollisionMapInfo wallInfo{};
	wallInfo.moveAmount_ = {velocity_.x, 0.0f, 0.0f};
	MapCollisionDetectionRight(wallInfo);
	MapCollisionDetectionLeft(wallInfo);
	ApplyCollisionMove(wallInfo);

	if (wallInfo.onWallCollision_) {
		dashAttackLeft_ = 0.0f;
	}

	if (dashAttackLeft_ <= 0.0f) {
		state_ = ActionState::Move;
		velocity_.x = std::clamp(velocity_.x, -kRimitRunSpeed, kRimitRunSpeed);
	}
}

bool Player::CanShoot() const {
	if (!CanAttack()) {
		return false;
	}
	if (reloading_) {
		return false;
	}
	return ammo_ > 0;
}

bool Player::TryConsumeAmmoAndStartReloadIfNeeded() {
	if (!CanAttack()) {
		return false;
	}
	if (reloading_) {
		return false;
	}
	if (ammo_ <= 0) {
		reloading_ = true;
		reloadLeft_ = kReloadTimeSec;
		return false;
	}

	--ammo_;
	if (ammo_ <= 0) {
		reloading_ = true;
		reloadLeft_ = kReloadTimeSec;
	}
	return true;
}

void Player::UpdateReload() {
	if (!reloading_) {
		return;
	}
	const float dt = 1.0f / 60.0f;
	reloadLeft_ = std::max(0.0f, reloadLeft_ - dt);
	if (reloadLeft_ <= 0.0f) {
		reloading_ = false;
		ammo_ = kAmmoMax;
	}
}

Vector3 Player::GetFacingDir() const {
	const float dir = (lrDirection_ == LRDirection::kRight) ? 1.0f : -1.0f;
	return {dir, 0.0f, 0.0f};
}

Vector3 Player::GetMuzzleWorldPos() const {
	const float dir = (lrDirection_ == LRDirection::kRight) ? 1.0f : -1.0f;
	Vector3 p = worldTransform_.translation_;
	p.x += dir * kShotMuzzleOffsetX;
	p.y += kShotMuzzleOffsetY;
	return p;
}

AABB Player::GetDashAttackAABB() const {
	// プレイヤー中心から前方へ kDashAttackHitRange だけ伸ばした箱
	const float dir = (lrDirection_ == LRDirection::kRight) ? 1.0f : -1.0f;
	const Vector3 p = worldTransform_.translation_;

	const float halfY = kHeight * 0.40f;
	const float halfZ = kWidth * 0.40f;

	AABB aabb{};
	if (dir > 0.0f) {
		aabb.min = {p.x, p.y - halfY, p.z - halfZ};
		aabb.max = {p.x + kDashAttackHitRange, p.y + halfY, p.z + halfZ};
	} else {
		aabb.min = {p.x - kDashAttackHitRange, p.y - halfY, p.z - halfZ};
		aabb.max = {p.x, p.y + halfY, p.z + halfZ};
	}
	return aabb;
}