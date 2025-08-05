#include "GameScene.h"

using namespace KamataEngine;

GameScene::~GameScene() {
	// 3Dモデルデータの開放
	delete playerModel_;
	// 3Dモデルデータの開放
	delete enemyModel_;
	// デスパーティクルモデルの開放
	delete particleModel_;
	// ブロックモデルデータの開放
	delete modelBlock_;
	// 天球モデルデータの開放
	delete modelSkydome_;
	// デバッグカメラの開放
	delete debugCamera_;
	// 自キャラの開放
	delete player_;
	// 天球の開放
	delete skydome_;
	// デスパーティクルの開放
	delete deathParticles_;
	// 敵キャラの開放
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	// マップチップフィールドの開放
	delete mapChipField_;

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
}

void GameScene::Initialize() {

	// 3Dモデルデータの生成
	playerModel_ = Model::CreateFromOBJ("player", true);
	// 3Dモデルデータの生成
	enemyModel_ = Model::CreateFromOBJ("enemy", true);
	// 敵を複数生成
	const int enemyCount = 3;
	for (int32_t i = 0; i < enemyCount; ++i) {
		Enemy* newEnemy = new Enemy();
		Vector3 enemyPosition = {float(i) * 3.0f, 1.0f, 0.0f}; // 一体ずつX方向にずらす
		newEnemy->Initialize(enemyModel_, &camera_, enemyPosition);
		enemies_.push_back(newEnemy);
	}
	particleModel_ = Model::CreateFromOBJ("particle", true);
	// ブロックモデルデータの生成
	modelBlock_ = Model::CreateFromOBJ("cube", true);
	// 天球モデルデータの生成
	modelSkydome_ = Model::CreateFromOBJ("sky_sphere", true);

	// カメラのfarZを適度に大きい値に
	camera_.farZ = 1000.0f;
	// カメラの初期化
	camera_.Initialize();
	camera_.translation_.z = -30.0f;

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);

	// マップチップフィールドの生成
	mapChipField_ = new MapChipField;
	// CSVファイルからマップデータを読み込み
	mapChipField_->LoadMapChipCsv("Resources/block.csv");

	// 自キャラの生成
	player_ = new Player;
	// 座標をマップチップ番号で指定
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 18);
	// 自キャラの初期化
	player_->Initialize(playerModel_, &camera_, playerPosition);
	// マップチップデータのセット
	player_->SetMapChipField(mapChipField_);

	// 仮の生成処理
	deathParticles_ = new DeathParticles;
	deathParticles_->Initialize(particleModel_, &camera_, playerPosition);

	// 天球の生成
	skydome_ = new Skydome;
	// 天球の初期化
	skydome_->Initialize();

	GenetateBlocks();

	// カメラコントローラーの初期化
	cameraController_ = new CameraController; // 生成
	cameraController_->Initialize();          // 初期化
	cameraController_->SetTarget(player_);    // 追従対象をセット
	cameraController_->Reset();               // リセット（瞬間合わせ）
}

void GameScene::Update() {
	// 自キャラの更新
	player_->Update();
	// 天球の更新
	skydome_->Update();
	// 敵キャラの更新
	for (Enemy* enemy : enemies_) {
		enemy->Update();
	}
	if (deathParticles_) {
		deathParticles_->Update();
	}
	// すべての当たり判定を行う
	CheckAllCollisions();

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
		// カメラコントローラーの更新
		cameraController_->Update();

		// カメラを controller から取得して camera_ に反映
		const Camera& controlledCam = cameraController_->GetCamera();
		camera_.matView = controlledCam.matView;
		camera_.matProjection = controlledCam.matProjection;

		// ここで行列転送も必要（たとえば TransferMatrix などが必要なら）
		camera_.TransferMatrix();
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
	// 天球の描画
	skydome_->Draw(&camera_);
	// 敵キャラの描画
	for (Enemy* enemy : enemies_) {
		enemy->Draw();
	}
	if (deathParticles_) {
		deathParticles_->Draw();
	}
	// ブロックの描画
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			modelBlock_->Draw(*worldTransformBlock, camera_);
		}
	}
}

void GameScene::GenetateBlocks() {
	// 要素数
	const uint32_t kNumBlockVirtical = mapChipField_->GetNumBlockVertical();
	const uint32_t kNumBlockHorizontal = mapChipField_->GetNumBlockHorizontal();
	// ブロック一個分の縦横幅
	// const float kBlockWidth = 1.0f;
	// const float kBlockHeight = 1.0f;
	// 要素数を変更する
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		// 1列の要素数を設定（横方向のブロック数）
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}

	// キューブの生成
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
			}
		}
	}
}

void GameScene::CheckAllCollisions() {
	#pragma region 
	{
		// 判定対象1と2の座標
		AABB aabb1, aabb2;

		//自キャラの座標
		aabb1 = player_->GetAABB();

		// 自キャラと敵球すべての当たり判定
		for (Enemy* enemy : enemies_) {
		// 敵弾の座標
			aabb2 = enemy->GetAABB();

			if (IsCollision(aabb1, aabb2)) {
				// 衝突応答処理
				// 自キャラの衝突時関数を呼び出す
				player_->OnCollision(enemy);
				// 敵の衝突時関数を呼び出す
				enemy->OnCollision(player_);
			}
		}
	}
	#pragma endregion
}
