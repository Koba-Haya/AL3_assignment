#pragma once
#include "KamataEngine.h"

using namespace KamataEngine;

// 前方宣言
class Player;

// 矩形
struct Rect {
	float left = 0.0f;   // 左端
	float right = 1.0f;  // 右端
	float bottom = 0.0f; // 下端
	float top = 1.0f;    // 上端
};

class CameraController {
public:
	void Initialize();
	void Update();
	void SetTarget(Player* target) { target_ = target; };
	void Reset();
	const KamataEngine::Camera& GetCamera() const { return camera_; }
	void SetMovableArea(Rect area) { movableArea_ = area; };

private:
	// カメラ
	KamataEngine::Camera camera_;
	// プレイヤー
	Player* target_ = nullptr;
	// 追従対象とカメラの座標の差（オフセット）
	Vector3 targetOffset_ = {0.0f, 0.0f, -15.0f};
	// カメラ移動範囲
	Rect movableArea_ = {0.0f, 100.0f, 0.0f, 100.0f};
	// カメラの目標座標
	KamataEngine::Vector3 targetCoordinates_;
	// 座標補間割合
	static inline const float kInterpolationRate = 0.9f;
	// 速度掛け率
	static inline const float kVelocityBias = 20.0f;
	// 追従対象の各方向へのカメラ移動範囲
	static inline const Rect margin = {-30.0f, 30.0f, -30.0f, 30.0f};
};
