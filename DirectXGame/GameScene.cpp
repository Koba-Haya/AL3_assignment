#include "GameScene.h"

using namespace KamataEngine;

GameScene::~GameScene() {
	// 3Dモデルデータの開放
	delete model_;
	// ブロックモデルデータの開放
	delete modelblock_;
	// デバッグカメラの開放
	delete debugCamera_;
	// 自キャラの開放
	delete player_;

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
}

void GameScene::Initialize() {
	// ファイル名を指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("uvChecker.png");

	// 3Dモデルデータの生成
	model_ = Model::Create();

	// ブロックモデルデータの生成
	modelblock_ = Model::Create();

	// カメラの初期化
	camera_.Initialize();

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);

	// 自キャラの生成
	player_ = new Player();
	// 自キャラの初期化
	player_->Initialize(model_, textureHandle_, &camera_);

	// 要素数
	const uint32_t kNumBlockVirtical = 10;
	const uint32_t kNumBlockHorizontal = 20;
	// ブロック一個分の縦横幅
	const float kBlockWidth = 2.0f;
	const float kBlockHeight = 2.0f;
	// 要素数を変更する
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}

	// キューブの生成
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if ((i + j) % 2 == 0) {
				worldTransformBlocks_[i][j] = nullptr; // 穴にする
				continue;
			}
			worldTransformBlocks_[i][j] = new WorldTransform();
			worldTransformBlocks_[i][j]->Initialize();
			worldTransformBlocks_[i][j]->translation_.x = kBlockWidth * j;
			worldTransformBlocks_[i][j]->translation_.y = kBlockHeight * i;
		}
	}
}

void GameScene::Update() {
	// 自キャラの更新
	player_->Update();
#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_1)) { // 例：キー1で切り替え
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	if (isDebugCameraActive_) {
		debugCamera_->Update();
		// DebugCamera から Camera を取得し、camera_ にコピー
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;

		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
	}

	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			// アフィン変換行列の作成
			Matrix4x4 blockAffineMatrix = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			// ワールド行列に代入
			worldTransformBlock->matWorld_ = blockAffineMatrix;
			// 定数バッファの転送
			worldTransformBlock->TransferMatrix();
		}
	}
}

void GameScene::Draw() {
	// 自キャラの描画
	player_->Draw();
	// ブロックの描画
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			modelblock_->Draw(*worldTransformBlock, camera_);
		}
	}
}
