#pragma once
#include "KamataEngine.h"
#include "Method.h"
#include "Player.h"
#include "Skydome.h"
#include <vector>

using namespace KamataEngine;

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
	// テクスチャハンドル
	UINT32 textureHandle_ = 0;
	// カメラ
	KamataEngine::Camera camera_;
	// 3DPlayerモデルデータ
	KamataEngine::Model* model_ = nullptr;
	// ブロックモデルデータ
	KamataEngine::Model* modelBlock_ = nullptr;
	// 天球モデルデータ
	KamataEngine::Model* modelSydome_ = nullptr;

	bool isDebugCameraActive_ = false;
	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// 自キャラ
	Player* player_ = nullptr;

	// 天球
	Skydome* skydome_ = nullptr;

	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
};
