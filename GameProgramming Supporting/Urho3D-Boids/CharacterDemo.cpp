//
// Copyright (c) 2008-2016 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/CheckBox.h>

#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Timer.h>



#include "Character.h"
#include "CharacterDemo.h"
#include "Touch.h"
#include "Boids.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(CharacterDemo)

BoidSet boidset;
int score;

static const StringHash E_CLIENTOBJECTAUTHORITY("ClientObjectAuthority");
// Identifier for the node ID parameter in the event data
static const StringHash PLAYER_ID("IDENTITY");
// Custom event on server, client has pressed button that it wants to start game
static const StringHash E_CLIENTISREADY("ClientReadyToStart");



CharacterDemo::CharacterDemo(Context* context) :
    Sample(context),
    firstPerson_(false)
{
	//TUTORIAL: TODO
}

CharacterDemo::~CharacterDemo()
{
}

void CharacterDemo::Start()
{
	
	score = 0;
	OpenConsoleWindow();
    // Execute base class startup
    Sample::Start();
    if (touchEnabled_)
        touch_ = new Touch(context_, TOUCH_SENSITIVITY);
	
	cameraNode_ = new Node(context_);
	Camera* camera = cameraNode_->CreateComponent<Camera>(LOCAL);
	cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
	camera->SetFarClip(300.0f);
	//TUTORIAL: TODO
	//can be removed for play button if you add a boolean for running and then a if statement for the cmaera node 
	//CreateServerScene();
	// Subscribe to necessary events
	SubscribeToEvents();
	// Set the mouse mode to use in the sample
	Sample::InitMouseMode(MM_RELATIVE);
	
	CreateMainMenu();

}

void CharacterDemo::CreateServerScene()
{
	//TUTORIAL: TODO
	//so we can access resources
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	scene_ = new Scene(context_);
	// Create scene subsystem components
	scene_->CreateComponent<Octree>(LOCAL);
	scene_->CreateComponent<PhysicsWorld>(LOCAL);
	
	//Create camera node and component
	//cameraNode_ = new Node(context_);
	Camera* camera = cameraNode_->CreateComponent<Camera>(LOCAL);
	cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
	camera->SetFarClip(300.0f);

	GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_,scene_, camera));

	// Create static scene content. First create a zone for ambient lighting and fog control
		Node* zoneNode = scene_->CreateChild("Zone");
	Zone* zone = zoneNode->CreateComponent<Zone>();
	zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
	zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
	zone->SetFogStart(100.0f);
	zone->SetFogEnd(300.0f);
	zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

	// Create a directional light with cascaded shadow mapping
	Node* lightNode = scene_->CreateChild("DirectionalLight",LOCAL);
	lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
	Light* light = lightNode->CreateComponent<Light>();
	light->SetLightType(LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);
	light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
	light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f,
		0.8f));
	light->SetSpecularIntensity(0.5f);

	// Create the floor object
	Node* floorNode = scene_->CreateChild("Floor",LOCAL);
	floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
	floorNode->SetScale(Vector3(200.0f, 1.0f, 200.0f));
	StaticModel* object = floorNode->CreateComponent<StaticModel>();
	object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	object->SetMaterial(cache ->GetResource<Material>("Materials/Stone.xml"));
	RigidBody* body = floorNode->CreateComponent<RigidBody>();
	// Use collision layer bit 2 to mark world scenery. This is what we will raycast against to prevent camera from going
		// inside geometry
	body->SetCollisionLayer(2);
	CollisionShape* shape = floorNode->CreateComponent<CollisionShape>();
	shape->SetBox(Vector3::ONE);
	//Creating walls
	Node* WallList[4];
	for (int x = 0; x < 4; x++)
	{
		WallList[x] = CreateWalls();
	 }
	//z axis walls
	WallList[1]->SetPosition(Vector3(0.0f, -0.5f, -100.0f));
	//x axis walls
	WallList[2]->SetPosition(Vector3(100.0f, -0.5f, 0.0f));
	WallList[2]->SetRotation(Quaternion(90.0f, 0.0f, 90.0f));
	WallList[3]->SetPosition(Vector3(-100.0f, -0.5f, 0.0f));
	WallList[3]->SetRotation(Quaternion(90.0f, 0.0f, 90.0f));

	// Create a water plane object that is as large as the terrain
	Graphics* graphics = GetSubsystem<Graphics>();
	waterNode_ = scene_->CreateChild("Water",LOCAL);
	waterNode_->SetScale(Vector3(2048.0f, 1.0f, 2048.0f));
	waterNode_->SetPosition(Vector3(0.0f, 90.0f, 0.0f));
	waterNode_->SetRotation(Quaternion(180.0f, 0.0, 0.0f));
	StaticModel* water = waterNode_->CreateComponent<StaticModel>();
	water->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
	water->SetMaterial(cache->GetResource<Material>("Materials/Water.xml"));
	// Set a different viewmask on the water plane to be able to hide it from the reflection camera
	water->SetViewMask(0x80000000);

	// Create a mathematical plane to represent the water in calculations
	waterPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f), waterNode_->GetWorldPosition());
	//Create a downward biased plane for reflection view clipping. Biasing is necessary to avoid too aggressive //clipping
	waterClipPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f),
		waterNode_->GetWorldPosition() - Vector3(0.0f, 0.01f, 0.0f));

	// Create camera for water reflection
	// It will have the same farclip and position as the main viewport camera, but uses a reflection plane to modify
	// its position when rendering
	reflectionCameraNode_ = cameraNode_->CreateChild();
	Camera* reflectionCamera = reflectionCameraNode_->CreateComponent<Camera>(LOCAL);
	reflectionCamera->SetFarClip(50.0);
	reflectionCamera->SetViewMask(0x7fffffff); // Hide objects with only bit 31 in the viewmask (the water plane)
	reflectionCamera->SetAutoAspectRatio(true);
	reflectionCamera->SetUseReflection(true);
	reflectionCamera->SetReflectionPlane(waterPlane_);
	reflectionCamera->SetUseClipping(true); // Enable clipping of geometry behind water plane
	reflectionCamera->SetClipPlane(waterClipPlane_);
	// The water reflection texture is rectangular. Set reflection camera aspect ratio to match
	reflectionCamera->SetAspectRatio((float)graphics->GetWidth() / (float)graphics->GetHeight());
	// View override flags could be used to optimize reflection rendering. For example disable shadows
	//reflectionCamera->SetViewOverrideFlags(VO_DISABLE_SHADOWS);
	// Create a texture and setup viewport for water reflection. Assign the reflection texture to the diffuse
	// texture unit of the water material
	int texSize = 1024;
	SharedPtr<Texture2D> renderTexture(new Texture2D(context_));
	renderTexture->SetSize(texSize, texSize, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
	renderTexture->SetFilterMode(FILTER_BILINEAR);
	RenderSurface* surface = renderTexture->GetRenderSurface();
	SharedPtr<Viewport> rttViewport(new Viewport(context_, scene_, reflectionCamera));
	surface->SetViewport(0, rttViewport);
	Material* waterMat = cache->GetResource<Material>("Materials/Water.xml");
	waterMat->SetTexture(TU_DIFFUSE, renderTexture);

	// Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
	// illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
	// generate the necessary 3D texture coordinates for cube mapping
	Node* skyNode = scene_->CreateChild("Sky",LOCAL);
	skyNode->SetScale(500.0f); // The scale actually does not matter
	Skybox* skybox = skyNode->CreateComponent<Skybox>();
	skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

	boidset.Initialise(cache, scene_);

}

void CharacterDemo::CreateClientScene()
{
	//TUTORIAL: TODO
	//so we can access resources
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	scene_ = new Scene(context_);
	// Create scene subsystem components
	scene_->CreateComponent<Octree>(LOCAL);
	scene_->CreateComponent<PhysicsWorld>(LOCAL);

	//Create camera node and component
	cameraNode_ = new Node(context_);
	Camera* camera = cameraNode_->CreateComponent<Camera>(LOCAL);
	cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
	camera->SetFarClip(300.0f);

	GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera));

	// Create static scene content. First create a zone for ambient lighting and fog control
	Node* zoneNode = scene_->CreateChild("Zone");
	Zone* zone = zoneNode->CreateComponent<Zone>();
	zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
	zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
	zone->SetFogStart(100.0f);
	zone->SetFogEnd(200.0f);
	zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

	// Create a directional light with cascaded shadow mapping
	Node* lightNode = scene_->CreateChild("DirectionalLight", LOCAL);
	lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
	Light* light = lightNode->CreateComponent<Light>();
	light->SetLightType(LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);
	light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
	light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f,
		0.8f));
	light->SetSpecularIntensity(0.5f);

	// Create the floor object
	Node* floorNode = scene_->CreateChild("Floor", LOCAL);
	floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
	floorNode->SetScale(Vector3(200.0f, 1.0f, 200.0f));
	StaticModel* object = floorNode->CreateComponent<StaticModel>();
	object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	object->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
	RigidBody* body = floorNode->CreateComponent<RigidBody>();
	// Use collision layer bit 2 to mark world scenery. This is what we will raycast against to prevent camera from going
	// inside geometry
	body->SetCollisionLayer(2);
	CollisionShape* shape = floorNode->CreateComponent<CollisionShape>();
	shape->SetBox(Vector3::ONE);

	//Creating walls
	Node* WallList[4];
	for (int x = 0; x < 4; x++)
	{
		WallList[x] = CreateWalls();
	}
	//z axis walls
	WallList[1]->SetPosition(Vector3(0.0f, -0.5f, -100.0f));
	//x axis walls
	WallList[2]->SetPosition(Vector3(100.0f, -0.5f, 0.0f));
	WallList[2]->SetRotation(Quaternion(90.0f, 0.0f, 90.0f));
	WallList[3]->SetPosition(Vector3(-100.0f, -0.5f, 0.0f));
	WallList[3]->SetRotation(Quaternion(90.0f, 0.0f, 90.0f));

	// Create a water plane object that is as large as the terrain
	Graphics* graphics = GetSubsystem<Graphics>();
	waterNode_ = scene_->CreateChild("Water", LOCAL);
	waterNode_->SetScale(Vector3(2048.0f, 1.0f, 2048.0f));
	waterNode_->SetPosition(Vector3(0.0f, 90.0f, 0.0f));
	waterNode_->SetRotation(Quaternion(180.0f, 0.0, 0.0f));
	StaticModel* water = waterNode_->CreateComponent<StaticModel>();
	water->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
	water->SetMaterial(cache->GetResource<Material>("Materials/Water.xml"));
	// Set a different viewmask on the water plane to be able to hide it from the reflection camera
	water->SetViewMask(0x80000000);

	// Create a mathematical plane to represent the water in calculations
	waterPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f), waterNode_->GetWorldPosition());
	//Create a downward biased plane for reflection view clipping. Biasing is necessary to avoid too aggressive //clipping
	waterClipPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f),
		waterNode_->GetWorldPosition() - Vector3(0.0f, 0.01f, 0.0f));

	// Create camera for water reflection
	// It will have the same farclip and position as the main viewport camera, but uses a reflection plane to modify
	// its position when rendering
	reflectionCameraNode_ = cameraNode_->CreateChild();
	Camera* reflectionCamera = reflectionCameraNode_->CreateComponent<Camera>(LOCAL);
	reflectionCamera->SetFarClip(50.0);
	reflectionCamera->SetViewMask(0x7fffffff); // Hide objects with only bit 31 in the viewmask (the water plane)
	reflectionCamera->SetAutoAspectRatio(true);
	reflectionCamera->SetUseReflection(true);
	reflectionCamera->SetReflectionPlane(waterPlane_);
	reflectionCamera->SetUseClipping(true); // Enable clipping of geometry behind water plane
	reflectionCamera->SetClipPlane(waterClipPlane_);
	// The water reflection texture is rectangular. Set reflection camera aspect ratio to match
	reflectionCamera->SetAspectRatio((float)graphics->GetWidth() / (float)graphics->GetHeight());
	// View override flags could be used to optimize reflection rendering. For example disable shadows
	reflectionCamera->SetViewOverrideFlags(VO_DISABLE_SHADOWS);
	// Create a texture and setup viewport for water reflection. Assign the reflection texture to the diffuse
	// texture unit of the water material
	int texSize = 1024;
	SharedPtr<Texture2D> renderTexture(new Texture2D(context_));
	renderTexture->SetSize(texSize, texSize, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
	renderTexture->SetFilterMode(FILTER_BILINEAR);
	RenderSurface* surface = renderTexture->GetRenderSurface();
	SharedPtr<Viewport> rttViewport(new Viewport(context_, scene_, reflectionCamera));
	surface->SetViewport(0, rttViewport);
	Material* waterMat = cache->GetResource<Material>("Materials/Water.xml");
	waterMat->SetTexture(TU_DIFFUSE, renderTexture);

	// Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
	// illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
	// generate the necessary 3D texture coordinates for cube mapping
	Node* skyNode = scene_->CreateChild("Sky", LOCAL);
	skyNode->SetScale(500.0f); // The scale actually does not matter
	Skybox* skybox = skyNode->CreateComponent<Skybox>();
	skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));
	//boidset.Initialise(cache, scene_);
}

void CharacterDemo::CreateCharacter()
{
	//TUTORIAL: TODO
}

void CharacterDemo::CreateInstructions()
{
	//TUTORIAL: TODO
}

void CharacterDemo::SubscribeToEvents()
{
	//TUTORIAL: TODO
	// Subscribe to Update event for setting the character controls
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(CharacterDemo,HandleUpdate));
	SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(CharacterDemo, HandlePostUpdate));
	SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(CharacterDemo, HandleClientConnected));
	SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(CharacterDemo, HandleClientDisconnected));
	SubscribeToEvent(E_PHYSICSPRESTEP, URHO3D_HANDLER(CharacterDemo, HandlePhysicsPreStep));
	SubscribeToEvent(E_CLIENTISREADY, URHO3D_HANDLER(CharacterDemo, HandleClientToServerReadyToStart));
	GetSubsystem<Network>()->RegisterRemoteEvent(E_CLIENTISREADY);
	SubscribeToEvent(E_CLIENTOBJECTAUTHORITY, URHO3D_HANDLER(CharacterDemo, HandleServerToClientObjectID));
	GetSubsystem<Network>()->RegisterRemoteEvent(E_CLIENTOBJECTAUTHORITY);

	
	
}

void CharacterDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	//TUTORIAL: TODO
	using namespace Update;
	// Take the frame time step, which is stored as a float
	float timeStep = eventData[P_TIMESTEP].GetFloat();
	// Do not move if the UI has a focused element (the console)
	//if (GetSubsystem<UI>()->GetFocusElement()) return;
	Input* input = GetSubsystem<Input>();
	//// Movement speed as world units per second
	const float MOVE_SPEED = 20.0f;
	// Mouse sensitivity as degrees per pixel
	const float MOUSE_SENSITIVITY = 0.1f;
	// Use this frame's mouse motion to adjust camera node yaw and pitch.
	// Clamp the pitch between -90 and 90 degrees
	IntVector2 mouseMove = input->GetMouseMove();
	yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
	pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
	pitch_ = Clamp(pitch_, -90.0f, 90.0f);
	// Construct new orientation for the camera scene node from
	// yaw and pitch. Roll is fixed to zero
	cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
	// Read WASD keys and move the camera scene node to the corresponding
	// direction if they are pressed, use the Translate() function
	// (default local space) to move relative to the node's orientation.
	if (input->GetKeyDown(KEY_W))
		cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED *
			timeStep);
	if (input->GetKeyDown(KEY_S))
		cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
	if (input->GetKeyDown(KEY_A))
		cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
	if (input->GetKeyDown(KEY_D))
		cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
	
	Network* network = GetSubsystem<Network>();
	Connection* serverConnection = network->GetServerConnection();
	//this is local and not on server
	if (clientObjectID_)
	{
		Node* ballNode = this->scene_->GetNode(clientObjectID_);
		if (ballNode)
		{
			
			const float CAMERA_DISTANCE = 3.0f;
			cameraNode_->SetPosition(ballNode->GetPosition() + cameraNode_->GetRotation()
				* Vector3::BACK * CAMERA_DISTANCE);
		/*	RigidBody* body = ballNode->GetComponent<RigidBody>();
			body->SetRotation(cameraNode_->GetRotation() * Quaternion(90.0f, 0.0f, 0.0f));
			serverConnection->SetRotation(cameraNode_->GetRotation());*/
		}
	}
	//check collision
	CheckCollision();
	
	
	if (input->GetKeyPress(KEY_M))
	{
		MenuVisable = !MenuVisable;
	}
	UI* ui = GetSubsystem<UI>();
	ui->GetCursor()->SetVisible(MenuVisable);
	window_->SetVisible(MenuVisable);
	if (input->GetKeyPress(KEY_F))
	{
		FrameInfo frameInfo = GetSubsystem<Renderer>()->GetFrameInfo();
		Log::WriteRaw("FPS: " + String(1.0 / frameInfo.timeStep_) + "\n");
	}
	
	


}

void CharacterDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
	//TUTORIAL: TODO
	UI* ui = GetSubsystem<UI>();
	ui->GetCursor()->SetVisible(MenuVisable);
	window_->SetVisible(MenuVisable);

}

void CharacterDemo::CreateMainMenu()
{
	// Set the mouse mode to use in the sample
	Sample::InitMouseMode(MM_RELATIVE);
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();
	UIElement* root = ui->GetRoot();
	XMLFile* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
	root->SetDefaultStyle(uiStyle); 
	//need to set default ui style
//Create a Cursor UI element.We want to be able to hide and show the main menu at will.When hidden, the mouse will control the camera, and when visible, the
//mouse will be able to interact with the GUI.
		SharedPtr<Cursor> cursor(new Cursor(context_));
	cursor->SetStyleAuto(uiStyle);
	ui->SetCursor(cursor);
	// Create the Window and add it to the UI's root node
	window_ = new Window(context_);
	root->AddChild(window_);
	// Set Window size and layout settings
	window_->SetMinWidth(384);
	window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
	window_->SetAlignment(HA_CENTER, VA_CENTER);
	window_->SetName("Window");
	window_->SetStyleAuto();
	//creating buttons
	Font* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
	//Button* StartButton = CreateButton("Start", 24, font, window_);
	Button* createServer = CreateButton("Create Server", 24, font, window_);
	Button* Button2 = CreateButton("Join Server", 24, font, window_);
	Button* JoinGameButton = CreateButton("Client:Join Game", 24, font, window_);
	Button* QuitButton = CreateButton("Quit", 24, font, window_);
	
	lineEdit1 = CreateLineEdit("", 24, font, window_);
	SubscribeToEvent(QuitButton, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleQuit));
	SubscribeToEvent(createServer, E_RELEASED, URHO3D_HANDLER(CharacterDemo, CreateServer));
	//SubscribeToEvent(StartButton, E_RELEASED, URHO3D_HANDLER(CharacterDemo,));
	SubscribeToEvent(Button2, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleConnect));
	SubscribeToEvent(JoinGameButton, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleClientJoinGame));
	
}

Button * CharacterDemo::CreateButton(const String & text, int pHeight, Font* font, Urho3D::Window * whichWindow)
{
	Button* button = whichWindow->CreateChild<Button>();
	button->SetMinHeight(pHeight);
	button->SetStyleAuto();

	Text* ButtonText = button->CreateChild<Text>();
	ButtonText->SetFont(font, 12);
	ButtonText->SetAlignment(HA_CENTER, VA_CENTER);
	ButtonText->SetText(text);
	whichWindow->AddChild(button);
	return button;
}

LineEdit * CharacterDemo::CreateLineEdit(const String & text, int pHeight, Font * font, Urho3D::Window * whichWindow)
{
	LineEdit* Lineedit;
	Lineedit = whichWindow->CreateChild<LineEdit>();
	Lineedit->SetMinHeight(pHeight);
	Lineedit->SetAlignment(HA_CENTER, VA_CENTER);
	Lineedit->SetText(text);
	whichWindow->AddChild(Lineedit);
	Lineedit->SetStyleAuto();

	return Lineedit;
}

void CharacterDemo::CheckCollision()
{
	Network* network = GetSubsystem<Network>();
	const Vector<SharedPtr<Connection> >& connections = network->GetClientConnections();
	//Server: go through every client connected
	for (unsigned i = 0; i < connections.Size(); ++i)
	{
	
		Connection* connection = connections[i];
		// Get the object this connection is controlling
		Node* ConeNode = serverObjects_[connection];
		// Client has no item connected
		if (!ConeNode) continue;
		SubscribeToEvent(ConeNode, E_NODECOLLISION, URHO3D_HANDLER(CharacterDemo, HandleNodeCollision));
		//printf("Id: %i \n", ConeNode->GetID());
	}
}

void CharacterDemo::HandleQuit(StringHash eventType, VariantMap& eventData)
{
	engine_->Exit();
}

void CharacterDemo::HandleConnect(StringHash eventType, VariantMap & eventData)
{
	//Clears scene, prepares it for receiving
	CreateClientScene();
	Network* network = GetSubsystem<Network>();
	String address = lineEdit1->GetText().Trimmed();
	if (address.Empty()) { address = "localhost"; }
	//Specify scene to use as a client for replication
	network->Connect(address, SERVER_PORT, scene_);

	MenuVisable = false;
}

void CharacterDemo::HandleDisconnect(StringHash eventType, VariantMap & eventData)
{
	Log::WriteRaw("HandleDisconnect has been pressed. \n");
	Network* network = GetSubsystem<Network>();
	Connection* serverConnection = network->GetServerConnection();
	// Running as Client
	if (serverConnection)
	{
		serverConnection->Disconnect();
		scene_->Clear(true, false);
		clientObjectID_ = 0;
	}
	// Running as a server, stop it
	else if (network->IsServerRunning())
	{
		network->StopServer();
		scene_->Clear(true, false);
	}
}

void CharacterDemo::CreateServer(StringHash eventType, VariantMap & eventData)
{
	Log::WriteRaw("(CreateServerFunction) A Server has been Created");
	Network* network = GetSubsystem<Network>();
	network->StartServer(SERVER_PORT);
	CreateServerScene();
	MenuVisable = false;

}

void CharacterDemo::HandleClientJoinGame(StringHash eventType, VariantMap & eventData)
{
	printf("Client has pressed START GAME \n");
	if (clientObjectID_ == 0) // Client is still observer
	{
		Network* network = GetSubsystem<Network>();
		Connection* serverConnection = network->GetServerConnection();
		if (serverConnection)
		{
			VariantMap remoteEventData;
			remoteEventData[PLAYER_ID] = 0;
			serverConnection->SendRemoteEvent(E_CLIENTISREADY, true, remoteEventData);
		}
	}
	MenuVisable = !MenuVisable;
}

Node * CharacterDemo::CreateControllableObject()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	Node* ClientCone = scene_->CreateChild("AClientClone");
	//ClientCone->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
	ClientCone->SetScale(1.0f);
	StaticModel* ConeObject = ClientCone->CreateComponent<StaticModel>();
	ConeObject->SetModel(cache->GetResource<Model>("Models/Cone.mdl"));
	ConeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneSmall.xml"));
	ConeObject->SetCastShadows(false);

	// Create the physics components
	RigidBody* body = ClientCone->CreateComponent<RigidBody>();
	body->SetMass(1.0f);
	body->SetTransform(Vector3(0.0f, 5.0f, 0.0f), Quaternion(90.0f, 0.0f, 0.0f));
	//body->SetFriction(1.0f);
	body->SetLinearFactor(Vector3::ZERO);
	body->SetAngularFactor(Vector3::ZERO);
	body->SetAngularVelocity(Vector3::ZERO);
	body->SetLinearVelocity(Vector3::ZERO);
	body->SetUseGravity(false);
	// motion damping so that the ball can not accelerate limitlessly
	body->SetLinearDamping(0.5f);
	body->SetAngularDamping(0.5f);
	body->SetCollisionLayer(2);
	
	CollisionShape* shape = ClientCone->CreateComponent<CollisionShape>();
	shape->SetConvexHull(ConeObject->GetModel());
	//SubscribeToEvent(ClientCone, E_NODECOLLISION, URHO3D_HANDLER(CharacterDemo, HandleNodeCollision));
	return ClientCone;

	

	
}

Node * CharacterDemo::CreateWalls()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	Node* WallNode = scene_->CreateChild("Wall", LOCAL);
	WallNode->SetPosition(Vector3(0.0f, -0.5f, 100.0f));
	WallNode->SetScale(Vector3(200.0f, 1.0f, 200.0f));
	WallNode->SetRotation(Quaternion(90.0f, 0.0f, 0.0f));
	StaticModel* wallobject = WallNode->CreateComponent<StaticModel>();
	wallobject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	wallobject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
	RigidBody* Wallbody = WallNode->CreateComponent<RigidBody>();
	// Use collision layer bit 2 to mark world scenery. This is what we will raycast against to prevent camera from going
	// inside geometry
	Wallbody->SetCollisionLayer(2);
	CollisionShape* wallshape = WallNode->CreateComponent<CollisionShape>();
	wallshape->SetBox(Vector3::ONE);
	return WallNode;
}

void CharacterDemo::HandleServerToClientObjectID(StringHash eventType, VariantMap & eventData)
{
	clientObjectID_ = eventData[PLAYER_ID].GetUInt();
	printf("Client ID : %i \n", clientObjectID_);

}

void CharacterDemo::HandleClientToServerReadyToStart(StringHash eventType, VariantMap & eventData)
{
	printf("Event sent by the Client and running on Server: Client is ready to start the game \n");
	using namespace ClientConnected;
	Connection* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
	// Create a controllable object for that client
	Node* newObject = CreateControllableObject();
	serverObjects_[newConnection] = newObject;
	

	// Finally send the object's node ID using a remote event
	VariantMap remoteEventData;
	remoteEventData[PLAYER_ID] = newObject->GetID();
	newConnection->SendRemoteEvent(E_CLIENTOBJECTAUTHORITY, true, remoteEventData);
}

void CharacterDemo::HandleClientConnected(StringHash eventType, VariantMap & eventData)
{
	Log::WriteRaw("(Connected) A Client has Connected");

	using namespace ClientConnected;
	// When a client connects, assign to a scene
	Connection* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
	newConnection->SetScene(scene_);
	
}

void CharacterDemo::HandleClientDisconnected(StringHash eventType, VariantMap & eventData)
{
	Log::WriteRaw("(Disconnected) A Client has Disconnected");
	using namespace ClientConnected;

}

Controls CharacterDemo::FromClientToServerControls()
{
	Input* input = GetSubsystem<Input>();
	Controls controls;
	//Check which button has been pressed, keep track
	controls.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
	controls.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
	controls.Set(CTRL_LEFT, input->GetKeyDown(KEY_A));
	controls.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D));
	controls.Set(1024, input->GetKeyDown(KEY_E));
	// mouse movements to server
	controls.yaw_ = yaw_;
	controls.pitch_ = pitch_;
	return controls;


}

void CharacterDemo::ProcessClientControls(float Timestep)
{
	
	Network* network = GetSubsystem<Network>();
	const Vector<SharedPtr<Connection> >& connections = network->GetClientConnections();
	//Server: go through every client connected
	for (unsigned i = 0; i < connections.Size(); ++i)
	{

		Connection* connection = connections[i];
		// Get the object this connection is controlling
		Node* ConeNode = serverObjects_[connection];
		// Client has no item connected
		if (!ConeNode) continue;
		RigidBody* body = ConeNode->GetComponent<RigidBody>();
		// Get the last controls sent by the client
		const Controls& controls = connection->GetControls();
		const float MoveSpeed = 15.0f;
		body->SetRotation(Quaternion(controls.pitch_,controls.yaw_, 0));
	
	

		if (controls.buttons_ & CTRL_FORWARD)
			ConeNode->Translate(Vector3::FORWARD * MoveSpeed* Timestep);
		if (controls.buttons_ & CTRL_BACK)
			ConeNode->Translate(Vector3::BACK * MoveSpeed * Timestep);
		if (controls.buttons_ & CTRL_LEFT)
			ConeNode->Translate(Vector3::LEFT * MoveSpeed * Timestep);
		if (controls.buttons_ & CTRL_RIGHT)
			ConeNode->Translate(Vector3::RIGHT * MoveSpeed * Timestep);
	}


}

void CharacterDemo::HandlePhysicsPreStep(StringHash eventType, VariantMap & eventData)
{
	using namespace Update;
	float Timestep = eventData[P_TIMESTEP].GetFloat();
	Network* network = GetSubsystem<Network>();
	Connection* serverConnection = network->GetServerConnection();
	
	// Client: collect controls
	if (serverConnection)
	{

		serverConnection->SetPosition(cameraNode_->GetPosition()); // send camera position too
		serverConnection->SetControls(FromClientToServerControls()); // send controls to server

	}
	// Server: Read Controls, Apply them if needed
	else if (network->IsServerRunning())
	{
	
		ProcessClientControls(Timestep); // take data from clients, process it
		boidset.Update(Timestep);
	}

}

void CharacterDemo::HandleClientFinishedLoading(StringHash eventType, VariantMap & eventData)
{
}

void CharacterDemo::HandleCustomEventByOlivier(StringHash eventType, VariantMap & eventData)
{
}

void CharacterDemo::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
	// Check collision contacts and see if character is standing on ground (look for a contact that has near vertical normal)
	using namespace NodeCollision;

	MemoryBuffer contacts(eventData[P_CONTACTS].GetBuffer());
	Node* Nodes = static_cast<Node*>(eventData[P_OTHERNODE].GetPtr());
	RigidBody* ConeBody = static_cast<RigidBody*>(eventData[P_BODY].GetPtr());
	RigidBody* OtherBody = static_cast<RigidBody*>(eventData[P_OTHERBODY].GetPtr());
	Node* Client = ConeBody->GetNode();
	Log::WriteRaw("Node: "+Nodes->GetName()+" \n");
	
	Vector3 TempPos;
	int number = Nodes->GetName().Compare("Cone");
	if(Nodes->GetName().Compare("Cone") == 0)
	{ 
		score++;
		
		printf("Client %i 's ", Client->GetID());
		printf("Score is: %i \n", score);
		Nodes->SetPosition(Vector3(130.0f,0.0f,0.0f));
		Nodes->SetScale(5.0f);
		OtherBody->SetLinearVelocity(Vector3::ZERO);
		OtherBody->SetMass(0.0f);
	}
	//ConeBody->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
	if (Nodes->GetName().Compare("Wall") == 0)
	{	
	
		if(Nodes->GetPosition().x_ > 0)
		{
			TempPos = ConeBody->GetPosition();
			TempPos.x_ -= 1.0f;
			ConeBody->SetPosition(TempPos);
		}
		else if (Nodes->GetPosition().x_ < 0)
		{
			TempPos = ConeBody->GetPosition();
			TempPos.x_ += 1.0f;
			ConeBody->SetPosition(TempPos);
		}
		else if (Nodes->GetPosition().z_ > 0)
		{
			TempPos = ConeBody->GetPosition();
			TempPos.z_ -= 1.0f;
			ConeBody->SetPosition(TempPos);
		}
		else if (Nodes->GetPosition().z_ < 0)
		{
			TempPos = ConeBody->GetPosition();
			TempPos.z_ += 1.0f;
			ConeBody->SetPosition(TempPos);
		}

	}
	if (Nodes->GetName().Compare("Floor") == 0)
	{
		
		TempPos = ConeBody->GetPosition();
		TempPos.y_ += 1.0f;
		ConeBody->SetPosition(TempPos);
	}
		
		
		
	

}