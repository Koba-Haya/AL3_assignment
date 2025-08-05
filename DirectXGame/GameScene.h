#pragma once
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Method.h"
#include "Player.h"
#include "Skydome.h"
#include <vector>

using namespace KamataEngine;

enum class Phase {
	kPlay,  // ゲームプレイ
	kDeath, // デス演出
};

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

	void GenetateBlocks();

	// すべての当たり判定を行う
	void CheckAllCollisions();

	void ChangePhase();

	// デスフラグのgetter
	bool IsFinished() const { return finished_; };

private:
	// カメラ
	KamataEngine::Camera camera_;
	// 3DPlayerモデルデータ
	KamataEngine::Model* playerModel_ = nullptr;
	// 3DEnemyモデルデータ
	KamataEngine::Model* enemyModel_ = nullptr;
	// 3DEnemyモデルデータ
	KamataEngine::Model* particleModel_ = nullptr;
	// ブロックモデルデータ
	KamataEngine::Model* modelBlock_ = nullptr;
	// 天球モデルデータ
	KamataEngine::Model* modelSkydome_ = nullptr;

	bool isDebugCameraActive_ = false;
	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// 自キャラ
	Player* player_ = nullptr;

	// 天球
	Skydome* skydome_ = nullptr;

	// 敵キャラ
	std::list<Enemy*> enemies_;

	// マップチップフィールド
	MapChipField* mapChipField_;

	// カメラコントローラー
	CameraController* cameraController_ = nullptr;

	DeathParticles* deathParticles_ = nullptr;

	// ゲームの現在のフェーズ
	Phase phase_;

	// 終了フラグ
	bool finished_ = false;

	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
};
