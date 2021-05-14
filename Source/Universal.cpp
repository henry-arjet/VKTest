#include<arjet/Universal.h>
#include<arjet/UBO.h>
#include<imgui.h>
#include<backends/imgui_impl_vulkan.h>
#include<backends/imgui_impl_sdl.h>

mat4 Universal::viewMatrix; //reference to the main camera's view matrix
Renderer Universal::renderer;
vector<GameObjectPtr> Universal::gameObjects;
bool Universal::mouseMode = true;
Camera* Universal::mainCamera;

GameObject* Universal::mainCameraObject;


int Universal::run() {
	//USE A DOUBLE POINTER. So have a pointer to universal::viewMatrix, and then have that be a pointer to mainCamera::viewMatrix
	//OR I could put that stuff in the start() function. So I load the camera, push Universal::viewMatrix, and then have each model's start() function apply that view matrix
	//Which I think would be faster in execution but a little more complex
	//Universal.viewMatrix must be a thing before I create models
	
	renderer.graphicsOptions.vsync = true;

	SceneLoader loader;
	loader.load();

	Universal::viewMatrix = mat4(0.0f); //Now that we have a camera, we need to add its veiwMatrix before we initialize the models with start()

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io; //I don't understand the second statement
	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForVulkan(renderer.window);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = renderer.inst;
	init_info.PhysicalDevice = renderer.physicalDevice;
	init_info.Device = renderer.device;
	init_info.QueueFamily = renderer.queueFamilyIndex;
	init_info.Queue = renderer.graphicsQueue;
	
	VkPipelineCache pipeCache;
	VkPipelineCacheCreateInfo pipeCacheInfo {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, NULL, 0, 0, NULL};

	vkCreatePipelineCache(renderer.device, &pipeCacheInfo, NULL, &pipeCache);
	init_info.PipelineCache = pipeCache;

	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	VkDescriptorPool pool;
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * (int)(sizeof(pool_sizes) / sizeof(*(pool_sizes))); //((int)(sizeof(_ARR) / sizeof(*(_ARR))))
	pool_info.poolSizeCount = (uint32_t)(sizeof(pool_sizes) / sizeof(*(pool_sizes)));
	pool_info.pPoolSizes = pool_sizes;
	assert(vkCreateDescriptorPool(renderer.device, &pool_info, NULL, &pool) == VK_SUCCESS);

	init_info.DescriptorPool = pool;
	init_info.Allocator = NULL;
	init_info.MinImageCount = renderer.swapchainImages.size();
	init_info.ImageCount = renderer.swapchainImages.size();

	 
	init_info.CheckVkResultFn = [](VkResult err){ 
		if (err == 0)
			return;
		fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
		if (err < 0)
			abort(); 
	};
	ImGui_ImplVulkan_Init(&init_info, renderer.renderPass);

	{//load fonts
		// Use any command queue
		VkCommandBuffer command_buffer = renderer.beginSingleTimeCommands();

		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

		renderer.endSingleTimeCommands(command_buffer);

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	Input::Init();
	Time::start();

	for (int i = 0; i < gameObjects.size(); i++) {
		cout << "Starting " << gameObjects[i]->name << endl;
		gameObjects[i]->start();
	}
	cout << offsetof(UniformBufferObject, UniformBufferObject::featureFlags) << "\n";
	Time::resetDelta(); //just so it doesn't count all the loading and starting in the first deltaTime.
	mainLoop();

	renderer.cleanModels();
	return 0;
}



bool show_demo_window = true;
bool show_another_window = false;


void Universal::mainLoop() {
	bool stillRunning = true;
while (stillRunning) {
	//set deltaTime and 'now' for this frame
	Time::resetDelta();

	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame(renderer.window);
	ImGui::NewFrame();

	//SDL Input
	SDL_Event event;
	uint eventCount = 0; //FOR DEBUG
	while (SDL_PollEvent(&event)) {

		++eventCount;
		switch (event.type) {
		case SDL_QUIT:
			stillRunning = false;
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				renderer.framebufferResized = true; //Does nothing right now. Not a priority
				break;
			default:
				break;
			}
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			input.ProcessKey(&event.key);
			break;
		default:
			break;
		}
	}

	//Cycle mouse mode vs keyboard only mode. Allows the mouse to be freed
	if (input.OnPress("Escape")) {
		if (mouseMode) {
			mouseMode = false;
			SDL_ShowCursor(1);
		}
		else {
			mouseMode = true;
			SDL_ShowCursor(0);
			SDL_WarpMouseInWindow(renderer.window, 50, 50);
		}
	}

	

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

	// Rendering
	

	//Get the new camera position to Universal
	viewMatrix = mainCamera->GetViewMatrix();

	//call update for every GameObject
	for (int i = 0; i < gameObjects.size(); i++) {
		gameObjects[i]->update();
	}

	ImGui::Render();
	renderer.drawData = ImGui::GetDrawData();

	renderer.drawFrame();

}//close while(stillRunning)
}//close mainLoop()
