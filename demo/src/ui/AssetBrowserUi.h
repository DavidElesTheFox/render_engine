#pragma once

#include <assets/AssetDatabase.h>
#include <scene/Scene.h>

namespace Ui
{
	class AssetBrowserUi
	{
	public:
		AssetBrowserUi(Assets::AssetDatabase& assets,
			Scene::Scene& scene)
			: _assets(assets)
			, _scene(scene)
		{

		}

		void onGui();
	private:
		Assets::AssetDatabase& _assets;
		Scene::Scene& _scene;
	};

}