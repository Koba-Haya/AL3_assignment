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

	HandleCeilingCollision(collisionMapInfo); // velocity_.y = 0.0f
	ApplyCollisionMove(collisionMapInfo);     // 実移動
	// 行列更新
	WorldTransformUpdate(worldTransform_);

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
}

void Player::Draw() {
	// 3Dモデルを描画
	model_->Draw(worldTransform_, *camera_);
}

void Player::Move() {

	/*-----------地面との当たり判定------------*/
	// 着地フラグ
	bool landing = false;

	// 地面との当たり判定
	// 下降中？
	if (velocity_.y < 0) {
		// Y座標が地面以下になったら着地
		if (worldTransform_.translation_.y <= 1.0f) {
			landing = true;
		}
	}

	/*--------------接地状態--------------*/
	if (onGround_) {
		// 左右移動操作
		if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_LEFT)) {
			// 左右加速
			Vector3 acceleration = {};
			if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
				if (velocity_.x < 0.0f) {
					// 速度と逆方向に入力中は急ブレーキ
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
					// 速度と逆方向に入力中は急ブレーキ
					velocity_.x *= (1.0f - kAttenuation);
				}
				acceleration.x -= kAcceleration;
				if (lrDirection_ != LRDirection::kLeft) {
					lrDirection_ = LRDirection::kLeft;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeTurn;
				}
			}

			// 加速/減速
			velocity_.x += acceleration.x;
			// 最大速度制限
			velocity_.x = std::clamp(velocity_.x, -kRimitRunSpeed, kRimitRunSpeed);
		} else {
			// 非入力時は速度減衰をかける
			velocity_.x *= (1.0f - kAttenuation);
		}

		if (Input::GetInstance()->PushKey(DIK_UP)) {
			// ジャンプ初速
			velocity_.x += 0.0f;
			velocity_.y += kJumpAcceleration;
			velocity_.z += 0.0f;
		}

		// ジャンプ開始
		if (velocity_.y > 0.0f) {
			// 空中状態に移行
			onGround_ = false;
		}
	} else {
		/*--------------空中--------------*/
		// 落下速度
		velocity_.x += 0.0f;
		velocity_.y += -kGravityAcceleration;
		velocity_.z += 0.0f;
		// 落下速度制限
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);

		if (landing) {
			// めり込み排斥
			worldTransform_.translation_.y = 1.0f;
			// 摩擦で横方向速度が減衰する
			velocity_.x *= (1.0f - kAttenuation);
			// 下方向速度をリセット
			velocity_.y = 0.0f;
			// 接地状態に移行
			onGround_ = true;
		}
	}

	// 移動
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;
}

const KamataEngine::WorldTransform& Player::GetWorldTransform() const { return worldTransform_; }

void Player::SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; };

void Player::MapCollisionDetection(CollisionMapInfo& info) { MapCollisionDetectionUp(info); }

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
	Rect hitRect = {0.0f,0.0f,0.0f,0.0f};

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
		// プレイヤーの上端座標（移動後）
		float movedTop = movedPosition.y + kHeight / 2.0f;
		// めり込み防止補正
		info.moveAmount_.y = std::max(0.0f, hitRect.bottom - (movedTop - info.moveAmount_.y));
		info.onCeilingCollision_ = true;
	}
}
// void MapcollisionDetectionDown(CollisionMapInfo& info);
// void MapcollisionDetectionRight(CollisionMapInfo& info);
// void MapcollisionDetectionLeft(CollisionMapInfo& info);

void Player::ApplyCollisionMove(const CollisionMapInfo& info) {
	// 判定された移動量分だけ実際に移動させる
	worldTransform_.translation_.x += info.moveAmount_.x;
	worldTransform_.translation_.y += info.moveAmount_.y;
	worldTransform_.translation_.z += info.moveAmount_.z;
}

void Player::HandleCeilingCollision(const CollisionMapInfo& info) {
	if (info.onCeilingCollision_) {
		DebugText::GetInstance()->ConsolePrintf("hit ceiling\n");
		velocity_.y = 0.0f;
	}
}