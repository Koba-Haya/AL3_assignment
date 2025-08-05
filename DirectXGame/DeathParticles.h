#pragma once
#include "KamataEngine.h"
#include "Method.h"
#include "TransformWorld.h"
#include <array>

using namespace KamataEngine;

class DeathParticles {
public:
	void Initialize(Model* model, Camera* camera, const Vector3& position);
	void Update();
	void Draw();

	// デスフラグのgetter
	bool IsFinished() const { return isFinished_; };

private:
	// モデル
	KamataEngine::Model* model_ = nullptr;
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;

	// パーティクルの個数
	static inline const uint32_t kNumParticles = 8;

	// 終了フラグ
	bool isFinished_ = false;
	// 経過時間カウント
	float counter_ = 0.0;

	// 色変更オブジェクト
	ObjectColor objectColor_;
	// 色の数値
	Vector4 color_;

	std::array<WorldTransform, kNumParticles> worldTransforms_;

	static inline const float kDuration = 2.0f;
	static inline const float kSpeed = 0.01f;
	static inline const float kAngleUnit = 360.0f / 8.0f;
};
