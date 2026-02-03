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

void Player::StartWire(const Vector3& targetWorldPos) {
	if (state_ == ActionState::Dead) {
		return;
	}

	// 回数制限（地面に触れるまで回復しない）
	if (wiresUsed_ >= maxWires_) {
		return;
	}

	Vector3 p = worldTransform_.translation_;
	Vector3 to = {targetWorldPos.x - p.x, targetWorldPos.y - p.y, targetWorldPos.z - p.z};
	float lenSq = to.x * to.x + to.y * to.y + to.z * to.z;

	// 近すぎる指定は無視（ガク防止）
	if (lenSq < 0.05f * 0.05f) {
		return;
	}

	// 距離制限（保険）
	if (lenSq > wire_.maxDistance * wire_.maxDistance) {
		float len = std::sqrt(lenSq);
		Vector3 dir = {to.x / len, to.y / len, to.z / len};
		wireTarget_ = {p.x + dir.x * wire_.maxDistance, p.y + dir.y * wire_.maxDistance, p.z + dir.z * wire_.maxDistance};
	} else {
		wireTarget_ = targetWorldPos;
	}

	wireLocked_ = true;
	state_ = ActionState::Wire;

	++wiresUsed_;

	// ワイヤー開始フレームは移動しない（見た目のガク防止）
	wireWarmupFrames_ = 1;

	// 開始時の縦速度由来のブレを消す
	velocity_.y = 0.0f;

	// 状態を一旦リセット
	onGround_ = false;
	onWall_ = false;
	jumpBufferLeft_ = 0.0f;
	coyoteLeft_ = 0.0f;
}

void Player::CancelWireKeepInertia() {
	if (state_ != ActionState::Wire) {
		return;
	}
	wireLocked_ = false;
	state_ = ActionState::Move;

	// このフレームでワイヤーが終わった印（着地瞬間の横滑り対策用）
	wireEndedThisFrame_ = true;
}

void Player::Update() {
	float dt = 1.0f / 60.0f;

	// 壁ジャンプ後の猶予更新
	if (wallDetachLeft_ > 0.0f) {
		wallDetachLeft_ = std::max(0.0f, wallDetachLeft_ - dt);
		if (wallDetachLeft_ == 0.0f) {
			wallDetachDirX_ = 0;
		}
	}

	switch (state_) {
	case ActionState::Move: {
		// SPACEでジャンプ（バッファ）
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			jumpBufferLeft_ = jumpBufferTime_;
		}

		// 通常移動（重力あり）
		Move();

		// X→Y の軸分離衝突
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

		// コヨーテ
		if (onGround_) {
			coyoteLeft_ = coyoteTime_;
		} else {
			coyoteLeft_ = std::max(0.0f, coyoteLeft_ - dt);
		}

		// ジャンプ処理
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

	case ActionState::Wire: {
		if (!wireLocked_) {
			state_ = ActionState::Move;
			break;
		}

		// ワイヤー開始フレームは移動しない（ガク防止）
		if (wireWarmupFrames_ > 0) {
			--wireWarmupFrames_;
			WorldTransformUpdate(worldTransform_);
			worldTransform_.TransferMatrix();
			break;
		}

		// ワイヤー中は重力なし
		Vector3 p = worldTransform_.translation_;
		Vector3 to = {wireTarget_.x - p.x, wireTarget_.y - p.y, wireTarget_.z - p.z};
		float lenSq = to.x * to.x + to.y * to.y + to.z * to.z;

		// 到達したら慣性を残してMoveへ（次フレームから重力あり）
		if (lenSq <= wire_.reachRadius * wire_.reachRadius) {
			CancelWireKeepInertia();
			WorldTransformUpdate(worldTransform_);
			worldTransform_.TransferMatrix();
			break;
		}

		float len = std::sqrt(lenSq);
		Vector3 dir = {to.x / len, to.y / len, to.z / len};

		// 1フレームで進む距離を「残り距離以下」にする（オーバーシュート防止）
		float step = std::min(wire_.speed, len);

		// 慣性として残す速度は、基本は一定速度のまま
		// ただし到達フレームだけ step が小さくなるので、慣性が不自然ならここを step に合わせてもOK
		velocity_.x = dir.x * step;
		velocity_.y = dir.y * step;
		velocity_.z = dir.z * step;

		// 実際に適用する移動量は step にする（重要）
		Vector3 move = {dir.x * step, dir.y * step, dir.z * step};

		// X→Y の衝突（ワイヤー中も同じ解決）
		CollisionMapInfo wallInfo{};
		wallInfo.moveAmount_ = {move.x, 0.0f, 0.0f};
		MapCollisionDetectionRight(wallInfo);
		MapCollisionDetectionLeft(wallInfo);
		ApplyCollisionMove(wallInfo);

		CollisionMapInfo verticalInfo{};
		verticalInfo.moveAmount_ = {0.0f, move.y, 0.0f};
		MapCollisionDetectionUp(verticalInfo);
		MapCollisionDetectionDown(verticalInfo);
		ApplyCollisionMove(verticalInfo);

		HandleCeilingCollision(verticalInfo);
		HandleGroundCollision(verticalInfo);
		HandleWallCollision(wallInfo);

		// 壁・床・天井に当たったら「そこで終了」
		if (wallInfo.onWallCollision_ || verticalInfo.onGroundCollision_ || verticalInfo.onCeilingCollision_) {
			if (wallInfo.onWallCollision_) {
				velocity_.x = 0.0f;
			}
			if (verticalInfo.onGroundCollision_ || verticalInfo.onCeilingCollision_) {
				velocity_.y = 0.0f;
			}
			CancelWireKeepInertia();
			WorldTransformUpdate(worldTransform_);
			worldTransform_.TransferMatrix();
			break;
		}

		// ここが「到達処理」：step が残り距離を食い切ったら終了
		// （reachRadius を使わなくても止まるが、併用してもOK）
		if (len <= wire_.reachRadius || step >= len - 1e-6f) {
			CancelWireKeepInertia();
		}

		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();

		break;
	}

	case ActionState::Dead:
		break;
	}
}

void Player::UpdateFreeze() {
	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Player::Draw() { model_->Draw(worldTransform_, *camera_); }

void Player::Move() {
	// 空中ではA/Dによる横移動をしない
	// ただし壁ジャンプ後のデタッチ中は、空中でも既存の速度（慣性）を減衰させるだけにする
	const bool canControlHorizontal = onGround_;

	if (canControlHorizontal) {
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
	} else {
		// 空中は入力で加速しない
		// 慣性を完全に残すなら何もしない
		// 完全停止にしたいなら次の1行を有効にする
		// velocity_.x = 0.0f;

		// ほどよく空中慣性を弱めたいなら減衰だけ
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
// --- あなたが貼ってくれたものをそのまま使う（変更なし）なので、ここでは省略しないで丸ごと貼る

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
		if (type != MapChipType::kBlock) {
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
		if (type != MapChipType::kBlock) {
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
			if (type != MapChipType::kBlock) {
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
			if (type != MapChipType::kBlock) {
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
