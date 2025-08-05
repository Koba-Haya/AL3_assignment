#pragma once
#include "KamataEngine.h"
#include "Method.h"
#include "TransformWorld.h"

using namespace KamataEngine;

class Player;

class Enemy {
public:
	void Initialize(Model* model, Camera* camera, const Vector3& position);
	void Update();
	void Draw();

	// AABBを取得
	AABB GetAABB();
	// 衝突応答
	void OnCollision(const Player* player);

private:
	// ワールド変換データ
	WorldTransform worldTransform_;
	// モデル
	Model* model_ = nullptr;
	// カメラ
	Camera* camera_ = nullptr;
	// 歩行の速さ
	static inline const float kWalkSpeed = 0.03f;
	// 速度
	Vector3 velocity_ = {};
	// 経過時間
	float walkTimer_ = 0.0f;
	// 最初の角度[度]
	static inline const float kWalkMotionAngleStart = -30.0f;
	// 最後の角度[度]
	static inline const float kWalkMotionAngleEnd = 30.0f;
	// アニメーションの周期となる時間[秒]
	static inline const float kWalkMotionTime = 2.0f;

	// キャラクターの当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;
};
