#pragma once
#include "KamataEngine.h"
#include "Method.h"
#include "TransformWorld.h"

using namespace KamataEngine;

static inline const float kAcceleration = 0.01f;
static inline const float kAttenuation = 0.05f;
static inline const float kRimitRunSpeed = 0.3f;
static inline const float kTimeTurn = 0.3f;

// 前方宣言
class MapChipField;

enum class LRDirection {
	kRight, // 右
	kLeft,  // 左
};

// マップとの当たり判定情報
struct CollisionMapInfo {
	bool onCeilingCollision_ = false;
	bool onGroundCollision_ = false;
	bool onWallCollision_ = false;
	Vector3 moveAmount_;
};

enum Corner {
	kRightBottom, // 右下
	kLeftBottom,  // 左下
	kRightTop,    // 右上
	kLeftTop,     // 左上
	kNumCorner    // 要素数
};

class Player {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">モデル</param>
	/// <param name="textureHandle">テクスチャハンドル</param>
	/// <param name="camera">カメラ</param>
	void Initialize(Model* model, Camera* camera, const Vector3& position);

	void Update();

	void Draw();

	void Move();

	const KamataEngine::WorldTransform& GetWorldTransform() const;
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; };
	void SetMapChipField(MapChipField* mapChipField);
	void MapCollisionDetection(CollisionMapInfo& info);
	Vector3 CornerPosition(const Vector3& center, Corner corner);
	void MapCollisionDetectionUp(CollisionMapInfo& info);
	// void MapCollisionDetectionDown(CollisionMapInfo& info);
	// void MapCollisionDetectionRight(CollisionMapInfo& info);
	// void MapCollisionDetectionLeft(CollisionMapInfo& info);
	void ApplyCollisionMove(const CollisionMapInfo& info);
	void HandleCeilingCollision(const CollisionMapInfo& info);

private:
	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	// モデル
	KamataEngine::Model* model_ = nullptr;
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;

	Vector3 velocity_ = {};

	LRDirection lrDirection_ = LRDirection::kRight;

	// 旋回開始時の角度
	float turnFirstRotationY_ = 0.0f;
	// 旋回タイマー
	float turnTimer_ = 0.0f;

	// 接地状態フラグ
	bool onGround_ = true;

	// マップチップによるフィールド
	MapChipField* mapChipField_ = nullptr;

	// 重力加速度（下方向）
	static inline const float kGravityAcceleration = 0.1f;
	// 最大落下速度（下方向）
	static inline const float kLimitFallSpeed = 0.3f;
	// ジャンプ初速（上方向）
	static inline const float kJumpAcceleration = 1.0f;

	// キャラクターの当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	static inline const float kBlank = 1.0f;
};
