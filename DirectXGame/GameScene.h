#pragma once
#include "KamataEngine.h"

class GameScene {
public:
	// デストラクタ
	~GameScene();

	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

private:
	// ImGuiで値を入力する変数
	float inputFloat3[3] = {0, 0, 0};

	// テクスチャハンドル
	UINT32 textureHandle_ = 0;
	// サウンドデータハンドル
	UINT32 soundDataHandle_ = 0;
	// 音声再生ハンドル
	UINT32 voiceHandle_ = 0;

	// スプライト
	KamataEngine::Sprite* sprite_ = nullptr;
	// 3Dモデル
	KamataEngine::Model* model_ = nullptr;

	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;
	// カメラ
	KamataEngine::Camera camera_;
	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
};
