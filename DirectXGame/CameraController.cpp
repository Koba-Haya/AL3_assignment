#define NOMINMAX
#include "CameraController.h"
#include "Player.h"

void CameraController::Initialize() {
	// カメラの初期化
	camera_.Initialize();
	// 移動範囲の指定
	movableArea_ = {0.0f, 100.0f, 0.0f, 100.0f};
	// 追加：初期ロックYを現在位置で初期化
	lockedY_ = camera_.translation_.y;
}

void CameraController::Update() {
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// X/Z は従来どおり（速度バイアス付き）
	targetCoordinates_.x = targetWorldTransform.translation_.x + targetOffset_.x + target_->GetVelocity().x * kVelocityBias;
	targetCoordinates_.z = targetWorldTransform.translation_.z + targetOffset_.z + target_->GetVelocity().z * kVelocityBias;

	// ▼ Yは“接地中のみ更新”。空中は最後に接地したYを保持
	if (!lockYWhileAir_ || target_->IsOnGround()) {
		lockedY_ = targetWorldTransform.translation_.y + targetOffset_.y;
	}
	targetCoordinates_.y = lockedY_;

	// なめらかに追従
	camera_.translation_ = Lerp(camera_.translation_, targetCoordinates_, kInterpolationRate);

	// 移動範囲制限（X/Yともにプレイヤー中心のマージン内に収める）
	camera_.translation_.x = std::max(camera_.translation_.x, targetWorldTransform.translation_.x + margin.left);
	camera_.translation_.x = std::min(camera_.translation_.x, targetWorldTransform.translation_.x + margin.right);
	camera_.translation_.y = std::max(camera_.translation_.y, targetWorldTransform.translation_.y + margin.bottom);
	camera_.translation_.y = std::min(camera_.translation_.y, targetWorldTransform.translation_.y + margin.top);

	camera_.UpdateMatrix();
}

void CameraController::Reset() {
	const WorldTransform& targetWT = target_->GetWorldTransform();
	camera_.translation_ = {targetWT.translation_.x + targetOffset_.x, targetWT.translation_.y + targetOffset_.y, targetWT.translation_.z + targetOffset_.z};
	// 追加：リセット時にロックYも合わせる
	lockedY_ = camera_.translation_.y;
}
