#define NOMINMAX
#include "Player.h"
#include "MapChipField.h"
#include <algorithm>
#include <numbers>
#include <assert.h>

using namespace KamataEngine;

void Player::Initialize(Model* model, Camera* camera, const Vector3& position) {
	// NULLポインタチェック
	assert(model);

	// 引数として受け取ったデータをメンバ変数に記録
	model_ = model;
	// textureHandle_ = textureHandle;
	camera_ = camera;

	// ワールド変換の初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
}

void Player::StartAttack() {
	if (state_ == ActionState::Dead)
		return;
	if (attackCooldownLeft_ > 0.0f)
		return; // ← クールタイム中は不可
	if (state_ == ActionState::AttackWindup || state_ == ActionState::AttackActive || state_ == ActionState::AttackRecovery)
		return;

	state_ = ActionState::AttackWindup;
	attackTimer_ = 0.0f;
	attackHitboxActive_ = false;

	attackCooldownLeft_ = attackCooldownSec_; // ← クールタイムセット
}

void Player::Update() {
	float dt = 1.0f / 60.0f;

	// ← 毎フレームでクールタイムを減らす
	if (attackCooldownLeft_ > 0.0f) {
		attackCooldownLeft_ = std::max(0.0f, attackCooldownLeft_ - dt);
	}

	switch (state_) {
	case ActionState::Move: {
		// 1) 押した瞬間をバッファに記録
		if (Input::GetInstance()->TriggerKey(DIK_UP)) {
			jumpBufferLeft_ = jumpBufferTime_;
		}

		Move();
		// 衝突情報を初期化
		CollisionMapInfo collisionMapInfo{};
		// 移動量に速度の値をコピー
		collisionMapInfo.moveAmount_ = velocity_;

		// マップ衝突チェック
		MapCollisionDetection(collisionMapInfo);

		ApplyCollisionMove(collisionMapInfo);     // 実移動
		HandleCeilingCollision(collisionMapInfo); // velocity_.y = 0.0f
		HandleGroundCollision(collisionMapInfo);
		HandleWallCollision(collisionMapInfo);

		// 2) コヨーテタイマー更新
		if (onGround_) {
			coyoteLeft_ = coyoteTime_; // 接地中はリフィル
		} else {
			coyoteLeft_ = std::max(0.0f, coyoteLeft_ - dt);
		}

		// 3) バッファ × 接地 or コヨーテ でジャンプ成立
		if (jumpBufferLeft_ > 0.0f && (onGround_ || coyoteLeft_ > 0.0f) && !collisionMapInfo.onWallCollision_) { // ← 追加
			velocity_.y = kJumpAcceleration;
			onGround_ = false;
			jumpBufferLeft_ = 0.0f;
			coyoteLeft_ = 0.0f;
		}

		// バッファ残量を減衰
		jumpBufferLeft_ = std::max(0.0f, jumpBufferLeft_ - dt);

		// 旋回制御
		if (turnTimer_ > 0.0f) {
			// 旋回タイマーをカウントダウン
			turnTimer_ -= 1.0f / 60.0f;

			// 左右の自キャラ角度テーブル
			float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};

			// 目標角度を取得
			float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

			// t: 進行割合（0.0 ～ 1.0）
			float t = 1.0f - std::clamp(turnTimer_ / kTimeTurn, 0.0f, 1.0f);
			float easedT = EaseInOut(t);

			// 補間
			worldTransform_.rotation_.y = std::lerp(turnFirstRotationY_, destinationRotationY, easedT);
		}
		// 攻撃開始入力は「押した瞬間」
		if ((Input::GetInstance()->TriggerKey(DIK_SPACE)) && attackCooldownLeft_ <= 0.0f) { // ← クールタイムチェック
			StartAttack();
			break;
		}
		// 行列更新
		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}
	case ActionState::AttackWindup: {
		attackTimer_ += dt;
		// 溜め中は幅(Z)を狭める
		worldTransform_.scale_.z = std::lerp(atk_.widthMax, atk_.widthMin, std::clamp(attackTimer_ / atk_.windup, 0.0f, 1.0f));

		CollisionMapInfo info{};
		info.moveAmount_ = {0.0f, velocity_.y, 0.0f};
		MapCollisionDetection(info);
		ApplyCollisionMove(info);
		HandleCeilingCollision(info);
		HandleWallCollision(info);

		if (attackTimer_ >= atk_.windup) {
			state_ = ActionState::AttackActive;
			attackTimer_ = 0.0f;
			attackHitboxActive_ = true;
		}
		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}

	case ActionState::AttackActive: {
		attackTimer_ += dt;

		// 幅(Z)を伸ばし戻す（widthMin → widthMax）
		{
			float tA = std::clamp(attackTimer_ / atk_.active, 0.0f, 1.0f);
			worldTransform_.scale_.z = std::lerp(atk_.widthMin, atk_.widthMax, tA);
		}

		// ★落下しない：Yは固定。Xだけ突進しつつ、壁衝突は解く
		float tA = std::clamp(attackTimer_ / std::max(atk_.active, 1e-6f), 0.0f, 1.0f);
		float k = EaseOutCubic(tA);
		float perSec = atk_.lungeDistance / std::max(atk_.active, 1e-6f);
		float step = perSec * (0.7f + 0.6f * k) * dt;
		float dir = (lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f;

		CollisionMapInfo info{};
		info.moveAmount_ = {dir * step, 0.0f, 0.0f}; // ← 縦0で通す
		MapCollisionDetection(info);
		ApplyCollisionMove(info);
		HandleCeilingCollision(info);
		HandleWallCollision(info);

		// 攻撃ヒットAABB（見た目に合わせて伸びる）
		BuildAttackAABB();

		if (attackTimer_ >= atk_.active) {
			state_ = ActionState::AttackRecovery;
			attackTimer_ = 0.0f;
			attackHitboxActive_ = false;
		}
		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}
	case ActionState::AttackRecovery: {
		attackTimer_ += dt;

		// 余韻では通常幅に戻しつつ……
		worldTransform_.scale_.z = std::lerp(worldTransform_.scale_.z, atk_.widthMax, 0.25f);

		// ★ここから重力を再開（通常の縦物理）
		if (!onGround_) {
			velocity_.y -= kGravityAcceleration;
			velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
		}

		CollisionMapInfo info{};
		info.moveAmount_ = {0.0f, velocity_.y, 0.0f};
		MapCollisionDetection(info);
		ApplyCollisionMove(info);
		HandleCeilingCollision(info);
		HandleGroundCollision(info); // ← 余韻では地面判定を戻す
		HandleWallCollision(info);

		if (attackTimer_ >= atk_.recovery) {
			state_ = ActionState::Move;
			attackTimer_ = 0.0f;
			worldTransform_.scale_.z = atk_.widthMax;
		}

		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}
	
	case ActionState::Dead:
		// 死亡処理（既存のまま）
		break;
	}
}

// void Player::Update() {
//	float dt = 1.0f / 60.0f; // 固定更新の想定（あなたのループに合わせて）
//	switch (state_) {
//	case ActionState::Idle:
//	case ActionState::Move: {
//		// 移動処理
//		Move();
//
//		// 衝突情報を初期化
//		CollisionMapInfo collisionMapInfo;
//		// 移動量に速度の値をコピー
//		collisionMapInfo.moveAmount_ = velocity_;
//
//		// マップ衝突チェック
//		MapCollisionDetection(collisionMapInfo);
//
//		ApplyCollisionMove(collisionMapInfo);     // 実移動
//		HandleCeilingCollision(collisionMapInfo); // velocity_.y = 0.0f
//		HandleGroundCollision(collisionMapInfo);
//		HandleWallCollision(collisionMapInfo);
//
//		// 旋回制御
//		if (turnTimer_ > 0.0f) {
//			// 旋回タイマーをカウントダウン
//			turnTimer_ -= 1.0f / 60.0f;
//
//			// 左右の自キャラ角度テーブル
//			float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
//
//			// 目標角度を取得
//			float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
//
//			// t: 進行割合（0.0 ～ 1.0）
//			float t = 1.0f - std::clamp(turnTimer_ / kTimeTurn, 0.0f, 1.0f);
//			float easedT = EaseInOut(t);
//
//			// 補間
//			worldTransform_.rotation_.y = std::lerp(turnFirstRotationY_, destinationRotationY, easedT);
//		}
//		// ここで攻撃入力（例：Z/J）をトリガー
//		if (Input::GetInstance()->PushKey(DIK_Z) || Input::GetInstance()->PushKey(DIK_J)) {
//			StartAttack();
//			break;
//		}
//		// 行列更新
//		WorldTransformUpdate(worldTransform_);
//		worldTransform_.TransferMatrix();
//		break;
//	}
//	case ActionState::Attack:
//		UpdateAttack(dt);
//		break;
//
//	case ActionState::Dead:
//		// 既存…
//		break;
//	}
// }

void Player::UpdateFreeze() {
	// 入力・移動・当たり判定・タイマーは進めない
	// 位置/回転/拡縮からワールド行列を作って転送だけ行う
	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Player::Draw() {
	// 3Dモデルを描画
	model_->Draw(worldTransform_, *camera_);
}

void Player::Move() {
	// 地上状態
	if (onGround_) {
		// 左右移動
		if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_LEFT)) {
			Vector3 acceleration = {};

			if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
				if (velocity_.x < 0.0f) {
					velocity_.x *= (1.0f - kAttenuation);
				}
				acceleration.x += kAcceleration;
				if (lrDirection_ != LRDirection::kRight) {
					lrDirection_ = LRDirection::kRight;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeTurn;
				}
			} else if (Input::GetInstance()->PushKey(DIK_LEFT)) {
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

			velocity_.x += acceleration.x;
			velocity_.x = std::clamp(velocity_.x, -kRimitRunSpeed, kRimitRunSpeed);
		} else {
			velocity_.x *= (1.0f - kAttenuation);
		}

		// ジャンプ入力
		if (Input::GetInstance()->TriggerKey(DIK_UP)) {
			velocity_.y = kJumpAcceleration;
		}
	}

	// 空中状態 or ジャンプ中の処理
	if (!onGround_) {
		velocity_.y -= kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
}

const KamataEngine::WorldTransform& Player::GetWorldTransform() const { return worldTransform_; }

void Player::SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; };

void Player::MapCollisionDetection(CollisionMapInfo& info) {
	MapCollisionDetectionUp(info);
	MapCollisionDetectionDown(info);
	MapCollisionDetectionRight(info);
	MapCollisionDetectionLeft(info);
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	static const Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0}, // 右下
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0}, // 左下
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0}, // 右上
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0}  // 左上
	};
	return {center.x + offsetTable[static_cast<uint32_t>(corner)].x, center.y + offsetTable[static_cast<uint32_t>(corner)].y, center.z + offsetTable[static_cast<uint32_t>(corner)].z};
}

void Player::MapCollisionDetectionUp(CollisionMapInfo& info) {
	if (info.moveAmount_.y <= 0.0f) {
		return;
	}

	std::array<Vector3, 2> positionsNew;
	Vector3 movedPosition = {worldTransform_.translation_.x + info.moveAmount_.x, worldTransform_.translation_.y + info.moveAmount_.y, worldTransform_.translation_.z + info.moveAmount_.z};

	// 左上・右上を取得
	positionsNew[0] = CornerPosition(movedPosition, Corner::kLeftTop);
	positionsNew[1] = CornerPosition(movedPosition, Corner::kRightTop);

	bool hit = false;
	Rect hitRect = {0.0f, 0.0f, 0.0f, 0.0f};

	// 左上チェック
	IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[0]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
	}

	// 右上チェック
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[1]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex); // 上書きでもOK（左右両方当たる場合どちらかで補正）
	}

	if (hit) {
		// セル境界をまたいだか確認
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition({worldTransform_.translation_.x, worldTransform_.translation_.y + kHeight / 2.0f, worldTransform_.translation_.z});

		if (indexSetNow.yIndex != indexSet.yIndex) {
			// プレイヤーの上端座標（移動後）
			float movedTop = movedPosition.y + kHeight / 2.0f;
			// めり込み防止補正
			info.moveAmount_.y = std::max(0.0f, hitRect.bottom - (movedTop - info.moveAmount_.y));
			info.onCeilingCollision_ = true;
		}
	}
}

void Player::MapCollisionDetectionDown(CollisionMapInfo& info) {
	if (info.moveAmount_.y >= 0.0f) {
		return;
	}

	std::array<Vector3, 2> positionsNew;
	Vector3 movedPosition = {worldTransform_.translation_.x + info.moveAmount_.x, worldTransform_.translation_.y + info.moveAmount_.y, worldTransform_.translation_.z + info.moveAmount_.z};

	// 左下・右下の座標を取得
	positionsNew[0] = CornerPosition(movedPosition, Corner::kLeftBottom);
	positionsNew[1] = CornerPosition(movedPosition, Corner::kRightBottom);

	bool hit = false;
	Rect hitRect = {};

	// 左下チェック
	IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[0]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
	}

	// 右下チェック
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[1]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
	}

	if (hit) {
		// セル境界をまたいだかを確認
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition({worldTransform_.translation_.x, worldTransform_.translation_.y - kHeight / 2.0f, worldTransform_.translation_.z});

		if (indexSetNow.yIndex != indexSet.yIndex) {
			// プレイヤーの下端（移動後）
			float movedBottom = movedPosition.y - kHeight / 2.0f;
			// めり込み防止補正
			info.moveAmount_.y = std::min(0.0f, hitRect.top - (movedBottom - info.moveAmount_.y));
			info.onGroundCollision_ = true;
		}
	}
}

void Player::MapCollisionDetectionRight(CollisionMapInfo& info) {
	if (info.moveAmount_.x <= 0.0f)
		return;

	const float eps = 0.001f;

	// --- まず「現在位置」で右壁にめり込んでないかを検査（デペネトレート）---
	{
		Vector3 cur = worldTransform_.translation_;
		Vector3 curRB = CornerPosition(cur, Corner::kRightBottom);
		Vector3 curRT = CornerPosition(cur, Corner::kRightTop);

		bool curHit = false;
		Rect curRect{};
		auto idx = mapChipField_->GetMapChipIndexSetByPosition(curRB);
		if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
			curRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
			curHit = true;
		}
		idx = mapChipField_->GetMapChipIndexSetByPosition(curRT);
		if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
			curRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
			curHit = true;
		}
		if (curHit) {
			float currentRight = cur.x + kWidth * 0.5f;
			float sep = curRect.left - currentRight - eps; // 通常は負（左へ押し戻し）
			if (sep > 0.0f) {
				info.moveAmount_.x = std::max(info.moveAmount_.x, sep);
				return;
			}
		}
	}

	// --- 通常の「これ以上進ませない」制限（既存ロジック）---
	Vector3 moved = {worldTransform_.translation_.x + info.moveAmount_.x, worldTransform_.translation_.y + info.moveAmount_.y, worldTransform_.translation_.z + info.moveAmount_.z};
	Vector3 rightBottom = CornerPosition(moved, Corner::kRightBottom);
	Vector3 rightTop = CornerPosition(moved, Corner::kRightTop);

	bool hit = false;
	Rect rect{};
	auto idx = mapChipField_->GetMapChipIndexSetByPosition(rightBottom);
	if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
		hit = true;
		rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
	}
	idx = mapChipField_->GetMapChipIndexSetByPosition(rightTop);
	if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
		hit = true;
		rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
	}
	if (hit) {
		float movedRight = moved.x + kWidth * 0.5f;
		info.moveAmount_.x = std::max(0.0f, rect.left - (movedRight - info.moveAmount_.x));
		info.clampedX_ = true;          
		info.onWallCollision_ = true;
	}
}

void Player::MapCollisionDetectionLeft(CollisionMapInfo& info) {
	if (info.moveAmount_.x >= 0.0f)
		return;

	const float eps = 0.001f;

	// デペネトレーション（現在位置で左壁に重なっていたら右へ押し戻す）
	{
		Vector3 cur = worldTransform_.translation_;
		Vector3 curLB = CornerPosition(cur, Corner::kLeftBottom);
		Vector3 curLT = CornerPosition(cur, Corner::kLeftTop);

		bool curHit = false;
		Rect curRect{};
		auto idx = mapChipField_->GetMapChipIndexSetByPosition(curLB);
		if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
			curRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
			curHit = true;
		}
		idx = mapChipField_->GetMapChipIndexSetByPosition(curLT);
		if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
			curRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
			curHit = true;
		}
		if (curHit) {
			float currentLeft = cur.x - kWidth * 0.5f;
			float sep = curRect.right - currentLeft + eps; // 通常は正（右へ押し戻し）
			if (sep > 0.0f) {
				info.moveAmount_.x = std::max(info.moveAmount_.x, sep);
				return;
			}
		}
	}

	// 既存の前進制限
	Vector3 moved = {worldTransform_.translation_.x + info.moveAmount_.x, worldTransform_.translation_.y + info.moveAmount_.y, worldTransform_.translation_.z + info.moveAmount_.z};
	Vector3 leftBottom = CornerPosition(moved, Corner::kLeftBottom);
	Vector3 leftTop = CornerPosition(moved, Corner::kLeftTop);

	bool hit = false;
	Rect rect{};
	auto idx = mapChipField_->GetMapChipIndexSetByPosition(leftBottom);
	if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
		hit = true;
		rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
	}
	idx = mapChipField_->GetMapChipIndexSetByPosition(leftTop);
	if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
		hit = true;
		rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
	}
	if (hit) {
		float movedLeft = moved.x - kWidth * 0.5f;
		info.moveAmount_.x = std::min(0.0f, rect.right - (movedLeft - info.moveAmount_.x));
		info.onWallCollision_ = true;
		info.clampedX_ = true;          
	}
}

void Player::ApplyCollisionMove(const CollisionMapInfo& info) {
	// 判定された移動量分だけ実際に移動させる
	worldTransform_.translation_.x += info.moveAmount_.x;
	worldTransform_.translation_.y += info.moveAmount_.y;
	worldTransform_.translation_.z += info.moveAmount_.z;
}

void Player::HandleCeilingCollision(const CollisionMapInfo& info) {
	if (info.onCeilingCollision_) {
		velocity_.y = 0.0f;
	}
}

void Player::HandleGroundCollision(const CollisionMapInfo& info) {
	if (info.onGroundCollision_) {
		velocity_.y = 0.0f;
		onGround_ = true;
	} else {
		onGround_ = false; // ←これがないとジャンプしても空中にならない！
	}
}

void Player::HandleWallCollision(const CollisionMapInfo& info) {
	if (info.onWallCollision_) {
		// ★“Xが実際にクランプされた”かつ“天井同時ヒットではない”ときだけ横速度をゼロに
		if (info.clampedX_ && !info.onCeilingCollision_) {
			velocity_.x = 0.0f;
		} else {
			// 天井同時ヒット時は横慣性を残す（微減衰したければ下行を有効化）
			// velocity_.x *= 0.98f;
		}
		// 壁接触中の多段ジャンプ抑止
		jumpBufferLeft_ = 0.0f;
		coyoteLeft_ = 0.0f;
		onGround_ = false; // 壁は地面じゃない
	}
}

Vector3 Player::GetWorldPosition() {
	Vector3 worldPos;

	// ワールド行列の平行移動成分を取得（=ワールド座標）
	worldPos.x = worldTransform_.matWorld_.m[3][0]; // Tx
	worldPos.y = worldTransform_.matWorld_.m[3][1]; // Ty
	worldPos.z = worldTransform_.matWorld_.m[3][2]; // Tz

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
}

void Player::UpdateAttack(float dt) {
	attackTimer_ += dt;

	const float T_w = atk_.windup;
	const float T_a = atk_.active;
	const float T_r = atk_.recovery;
	const float T_all = T_w + T_a + T_r;

	// 基本スケール（通常スケール）
	const Vector3 baseScale = {1.0f, 1.0f, 1.0f};

	// ========== 1) Windup：判定なし＋横幅だけ縮める ==========
	if (attackTimer_ < T_w) {
		float t = attackTimer_ / T_w; // 0→1
		float sx = std::lerp(1.0f, atk_.widthMin / atk_.widthMax, t);
		worldTransform_.scale_.x = sx; // 横を絞る
		worldTransform_.scale_.y = baseScale.y;
		worldTransform_.scale_.z = baseScale.z;

		attackHitboxActive_ = false; // ★ 判定OFF
	}

	// ========== 2) Active：判定あり＋横幅を戻しつつ前進 ==========
	else if (attackTimer_ < T_w + T_a) {
		float tA = (attackTimer_ - T_w) / T_a; // 0→1
		float k = EaseOutCubic(tA);            // 0→1（前半強め）

		// 横幅を Min→Max へ戻す（見た目で“伸びる”）
		float sx = std::lerp(atk_.widthMin / atk_.widthMax, 1.0f, k);
		worldTransform_.scale_.x = sx;
		worldTransform_.scale_.y = baseScale.y;
		worldTransform_.scale_.z = baseScale.z;

		// 前方向（+X を前。必要なら座標系に合わせて調整）
		Vector3 forward = {1.0f, 0.0f, 0.0f};
		Matrix4x4 rotY = MakeRotateYMatrix(worldTransform_.rotation_.y);
		forward = TransformNormal(forward, rotY);

		// 少し前進（成分ごとにスカラー倍）
		float step = (atk_.lungeDistance * k * dt / std::max(T_a, 1e-6f));
		worldTransform_.translation_.x += forward.x * step;
		worldTransform_.translation_.y += forward.y * step;
		worldTransform_.translation_.z += forward.z * step;

		// ★ この区間だけ攻撃判定ON＋AABBを生成
		attackHitboxActive_ = true;
		BuildAttackAABB();
	}

	// ========== 3) Recovery：判定なし＋通常に戻す ==========
	else {
		attackHitboxActive_ = false; // ★ 判定OFF
		// （必要ならふわっと戻す補間でもOK。ここでは即戻す）
		worldTransform_.scale_ = baseScale;

		if (attackTimer_ >= T_all) {
			attackTimer_ = 0.0f;
			state_ = ActionState::Move;
		}
	}

	// 行列確定
	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Player::BuildAttackAABB() {
	// Active 中前提
	float tA = std::clamp(attackTimer_ / std::max(atk_.active, 1e-6f), 0.0f, 1.0f);

	// 長さは rangeMin → rangeMax に伸びる
	float rangeNow = std::lerp(atk_.rangeMin, atk_.rangeMax, tA);

	// 幅は widthMin → widthMax に伸びる（見た目と合わせる）
	float widthNow = std::lerp(atk_.widthMin, atk_.widthMax, tA);

	// 基準位置（プレイヤー中心）
	const Vector3 p = worldTransform_.translation_;

	// 前方方向は +X / -X（左右で切り替え）
	float dir = (lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f;

	// 攻撃箱の中心は、前方に半分オフセット
	float halfLen = rangeNow * 0.5f;
	Vector3 center = {p.x + dir * (halfLen + 0.5f * kWidth), p.y, p.z};

	// AABB を作る（X=長さ方向, Z=幅, Y=高さ）
	float hx = halfLen;
	float hy = atk_.height * 0.5f;
	float hz = widthNow * 0.5f;

	attackAabb_.min = {center.x - hx, center.y - hy, center.z - hz};
	attackAabb_.max = {center.x + hx, center.y + hy, center.z + hz};
}