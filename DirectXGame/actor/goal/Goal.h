#pragma once
#include "KamataEngine.h"
#include "Method.h"
#include "TransformWorld.h"

using namespace KamataEngine;

// 通り抜け可能な到達トリガー兼モデル表示
class Goal {
public:

	// model … 外から渡す（ブロックモデルなどを流用OK）
	// pos   … ワールド座標（中心）
	// scale … 見た目スケール（1.0f 基準）
	void Initialize(Model* model, const Vector3& pos);

	void Update(/* float dt */);

	void Draw(Camera& camera);

	const AABB& GetAABB() const { return aabb_; }
	const Vector3& GetPosition() const { return worldTransform_.translation_; }
	bool IsActive() const { return active_; }
	void SetActive(bool v) { active_ = v; }

	// 位置・スケールを後から変えたらAABB更新を忘れず呼ぶ
	void SetPosition(const Vector3& p) {
		worldTransform_.translation_ = p;
		worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
		worldTransform_.TransferMatrix();
		RebuildAABB_();
	}
	void SetScale(const Vector3& s) {
		worldTransform_.scale_ = s;
		worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
		worldTransform_.TransferMatrix();
		RebuildAABB_();
	}

private:
	void RebuildAABB_() {
		// 中心±(0.5*scale) を単位キューブ基準でAABB化
		const Vector3 c = worldTransform_.translation_;
		const Vector3 r = {0.5f * worldTransform_.scale_.x, 0.5f * worldTransform_.scale_.y, 0.5f * worldTransform_.scale_.z};
		aabb_.min = {c.x - r.x, c.y - r.y, c.z - r.z};
		aabb_.max = {c.x + r.x, c.y + r.y, c.z + r.z};
	}

private:
	WorldTransform worldTransform_{};
	Model* model_ = nullptr; // 非所有（GameSceneが持っているモデルを借用）
	AABB aabb_{};
	bool active_ = true;
};