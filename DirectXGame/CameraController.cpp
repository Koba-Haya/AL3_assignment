#define NOMINMAX
#include "CameraController.h"
#include "Player.h"

void CameraController::Initialize() {
	// カメラの初期化
	camera_.Initialize();
	// 移動範囲の指定
	movableArea_ = {0.0f, 100.0f, 0.0f, 100.0f};
}

void CameraController::Update() {
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// 追従対象とオフセットと追従対象の速度からカメラの目標座標を計算
	targetCoordinates_ = {
	    targetWorldTransform.translation_.x + targetOffset_.x + target_->GetVelocity().x * kVelocityBias,
	    targetWorldTransform.translation_.y + targetOffset_.y + target_->GetVelocity().y * kVelocityBias,
	    targetWorldTransform.translation_.z + targetOffset_.z + target_->GetVelocity().z * kVelocityBias};
	// 座標補間によりゆったり追従
	camera_.translation_ = Lerp(camera_.translation_, targetCoordinates_, kInterpolationRate);
	// 移動範囲制限
	camera_.translation_.x = std::max(camera_.translation_.x, targetWorldTransform.translation_.x+ margin.left);
	camera_.translation_.x = std::min(camera_.translation_.x, targetWorldTransform.translation_.x + margin.right);
	camera_.translation_.y = std::max(camera_.translation_.y, targetWorldTransform.translation_.y + margin.bottom);
	camera_.translation_.y = std::min(camera_.translation_.y, targetWorldTransform.translation_.y + margin.top);
	// 行列を更新する
	camera_.UpdateMatrix();
	printf("cam.x=%.2f  tgt.x=%.2f\n", camera_.translation_.x, targetCoordinates_.x);
}

void CameraController::Reset() {
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// 追従対象とオフセットからカメラの座標を計算
	camera_.translation_ = {targetWorldTransform.translation_.x + targetOffset_.x, targetWorldTransform.translation_.y + targetOffset_.y, targetWorldTransform.translation_.z + targetOffset_.z};
}
