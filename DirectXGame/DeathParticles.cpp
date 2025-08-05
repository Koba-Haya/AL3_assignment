#include "DeathParticles.h"
#include <numbers>
#include <algorithm>

using namespace KamataEngine;

void DeathParticles::Initialize(Model* model, Camera* camera, const Vector3& position) {
	// 引数として受け取ったデータをメンバ変数に記録
	model_ = model;
	// textureHandle_ = textureHandle;
	camera_ = camera;

	// ワールド変換の初期化
	for (WorldTransform& worldTransform : worldTransforms_) {
		worldTransform.Initialize();
		worldTransform.translation_ = position;
	}

	objectColor_.Initialize();
	color_ = {1.0f, 1.0f, 1.0f, 1.0f};
}

void DeathParticles::Update() {
	// 終了なら何もしない
	if (isFinished_) {
		return;
	}

	// 各パーティクルのワールド変換を更新
	for (WorldTransform& worldTransform : worldTransforms_) {
		// 必要なら位置や回転の更新処理をここに書いても良い
		for (uint32_t i = 0; i < kNumParticles; ++i) {
			// 基本の速度ベクトル（右方向）
			Vector3 velocity = {kSpeed, 0.0f, 0.0f};

			// 角度を計算（ラジアン単位）
			float angleDeg = kAngleUnit * i;
			float angleRad = angleDeg * (std::numbers::pi_v<float> / 180.0f);

			// Z軸回転行列を作成
			Matrix4x4 matrixRotation = MakeRotateZMatrix(angleRad);

			// 回転させた速度ベクトルを得る
			velocity = Transform(velocity, matrixRotation);

			// 移動処理
			worldTransforms_[i].translation_.x += velocity.x;
			worldTransforms_[i].translation_.y += velocity.y;
			worldTransforms_[i].translation_.z += velocity.z;
		}

		// アフィン行列の計算と転送（VRAM）
		WorldTransformUpdate(worldTransform); // 行列の更新
		worldTransform.TransferMatrix(); // VRAMに転送
	}

	// カウンターを1フレーム分の秒数進める
	counter_ += 1.0f / 60.0f;

	// 存続時間の上限に達したら
	if (counter_ >= kDuration) {
		counter_ = kDuration;
		// 終了扱いにする
		isFinished_ = true;
	}

	    // フェードアウト用のアルファ値を計算
	float t = counter_ / kDuration;
	color_.w = std::clamp(1.0f - t, 0.0f, 1.0f);

	// 色を GPU に反映
	objectColor_.SetColor(color_);
}

void DeathParticles::Draw() {
	// 終了なら何もしない
	if (isFinished_) {
		return;
	}
	for (const WorldTransform& worldTransform : worldTransforms_) {
		// 第3引数に &objectColor_ を渡すと色変更が反映される
		model_->Draw(worldTransform, *camera_, &objectColor_);
	}
}