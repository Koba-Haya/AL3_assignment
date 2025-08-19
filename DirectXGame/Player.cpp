#define NOMINMAX
#include "Player.h"
#include "MapChipField.h"
#include <algorithm>
#include <numbers>

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

void Player::Update() {
	// 移動処理
	Move();

	// 衝突情報を初期化
	CollisionMapInfo collisionMapInfo;
	// 移動量に速度の値をコピー
	collisionMapInfo.moveAmount_ = velocity_;

	// マップ衝突チェック
	MapCollisionDetection(collisionMapInfo);

	ApplyCollisionMove(collisionMapInfo);     // 実移動
	HandleCeilingCollision(collisionMapInfo); // velocity_.y = 0.0f
	HandleGroundCollision(collisionMapInfo);
	HandleWallCollision(collisionMapInfo);

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

	// 行列更新
	WorldTransformUpdate(worldTransform_);
	DebugText::GetInstance()->ConsolePrintf("onGround: %s\n", onGround_ ? "true" : "false");
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
		if (Input::GetInstance()->PushKey(DIK_UP)) {
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

	Vector3 movedPosition = {worldTransform_.translation_.x + info.moveAmount_.x, worldTransform_.translation_.y + info.moveAmount_.y, worldTransform_.translation_.z + info.moveAmount_.z};
	Vector3 rightBottom = CornerPosition(movedPosition, Corner::kRightBottom);
	Vector3 rightTop = CornerPosition(movedPosition, Corner::kRightTop);

	bool hit = false;
	Rect hitRect = {};

	// 下チェック
	IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(rightBottom);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
	}

	// 上チェック
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(rightTop);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
	}

	if (hit) {
		float movedRight = movedPosition.x + kWidth / 2.0f;
		info.moveAmount_.x = std::max(0.0f, hitRect.left - (movedRight - info.moveAmount_.x));
		info.onWallCollision_ = true;
	}
}

void Player::MapCollisionDetectionLeft(CollisionMapInfo& info) {
	if (info.moveAmount_.x >= 0.0f)
		return;

	Vector3 movedPosition = {worldTransform_.translation_.x + info.moveAmount_.x, worldTransform_.translation_.y + info.moveAmount_.y, worldTransform_.translation_.z + info.moveAmount_.z};
	Vector3 leftBottom = CornerPosition(movedPosition, Corner::kLeftBottom);
	Vector3 leftTop = CornerPosition(movedPosition, Corner::kLeftTop);

	bool hit = false;
	Rect hitRect = {};

	// 下チェック
	IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(leftBottom);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
	}

	// 上チェック
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(leftTop);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
	}

	if (hit) {
		float movedLeft = movedPosition.x - kWidth / 2.0f;
		info.moveAmount_.x = std::min(0.0f, hitRect.right - (movedLeft - info.moveAmount_.x));
		info.onWallCollision_ = true;
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
		velocity_.x = 0.0f;
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
