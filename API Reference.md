# API Reference

|namespace|class|function|parameter|return|description|
|:---:|:---:|:---:|:---:|:---:|:---:|
|Core|Initializer|getInstance|N/A|VkInstance||
|Core|Initializer|getDevice|N/A|VkDevice||
|Core|Initializer|getPhysicalDevice|N/A|VkPhysicalDevice||
|Core|Initializer|getGraphicsQueue|N/A|VkQueue||
|Core|Initializer|getPresentQueue|N/A|VkQueue||
|Core|Initializer|isGlfwInitialized|N/A|bool|If Glfw has initialized, return true, else, return false|
|Core|Initializer|getGraphicsQueueFamilyIndex|N/A|uint32_t||
|Core|Initializer|getPresentQueueFamilyIndex|N/A|uint32_t||
|Core|Initializer|Initializer|std::string appName|N/A|Use param:appName as the pApplicationName|
|Core|logger|logger|std::ostream& out|N/A|The logs will output by param:out, default: std::clog|
|Core|logger|traceLog|logger::LogLevel level, std::string message, std::source_location sl|N/A|Output a log carries param:message with loglevel: param:level, and param:sl for file, function, line & column|
|Core|logger|varLog|logger::LogLevel level, std::string varName, \[any] varValue, std::source_location sl|N/A|Output a log with format:"param:varName = param:varValue"|
|Core|logger|formatLog|logger::LogLevel level, std::string format, std::source_location sl, \[args]...|N/A|C-style format function|
|Core|Clock|Clock|std::function<void()> loop, long long FPS|N/A|Create a clock that runs param:loop every single frame, FPS is 60 in default|
|Core|Clock|run|N/A|N/A|Runs the loop function once and does other pragmas|
|Core|Clock|stop|N/A|N/A|Stops the clock violently, could cause crash|
|Core|Clock|fps|N/A|double|Returns the current fps|
|Window|InputState|isKeyPressed|int key|bool|Get the input state|
|Window|InputState|isKeyJustPressed|int key|bool|Same as above|
|Window|InputState|getMousePosition|N/A|vec2|Same as above|
|Window|InputState|getMouseDelta|N/A|vec2|Same as above|
|Window|InputState|isMouseButtonPressed|int button|bool|Same as above|
|Window|InputState|isMouseButtonJustPressed|int button|bool|Same as above|
|Window|InputState|getScrollOffset|N/A|double|Same as above|
|Window|Handle|shouldClose|N/A|bool|If the Window should close, return true, else, return false|
|Window|Handle|setTitle|std::string title|N/A|Set the window title to param:title|
|Window|Handle|getWidth|N/A|int|Get the width of the window|
|Window|Handle|getHeight|N/A|int|Get the height of the window|
|Window|Handle|close|N/A|N/A|Close the window|
|Window|Handle|getInputState|N/A|Window::InputState|Get the current InputState of the window|
|Window|Handle|setRenderCallback|std::function\<void(Render::Painter&)> callback|N/A|Set the callback of the window|
|Window|Handle|setKeyCallback|std::function\<void(int key,int scancode,int action,int mods)>|N/A|Same as above|
|Window|Handle|setMouseButtonCallback|std::function\<void(int button,int action,int mods)>|N/A|Same as above|
|Window|Handle|setCursorPosCallback|std::function\<void(double x,double y)>|N/A|Same as above|
|Window|Handle|setScrollCallback|std::function\<void(double xoffset,double yoffset)>|N/A|Same as above|
|Window|WindowManager|instance|N/A|WindowManager&|Get the instance of the WindowManager|
|Window|WindowManager|create|Core::Initializer, int width, int height, std::string title|std::unique_ptr\<Window::Handle>|Create a window with width=param:width, height=param:height and title=param:title|
|Window|WindowManager|destroy|std::unique_ptr\<Window::Handle>|N/A|Close the window|
|Window|WindowManager|count|N/A|size_t|Return the number of active windows|
|Render|Painter|drawLine|vec2 p1, vec2 p2, vec3/vec4/Core::Color color, float width|N/A|Draw a line between param:p1 and param:p2, with color param:color, width is 0.001f in default (which is, in NDC coordinate, a line)|
|Render|Painter|drawPoint|vec2 p, vec3/vec4/Core::Color color, float size|N/A|Draw a square at param:p with color param:color, the size is 0.001f in default (which is, in NDC coordinate, a point)|
|Render|Painter|drawTriangle|vec2 p1, vec2 p2, vec2 p3, (vec3/vec4/Core::Color color)/(vec3/vec4/Core::Color c1,c2,c3)|N/A|Draw a triangle|
|Render|Painter|drawRect|vec2 pos, vec2 size, vec3/vec4/Core::Color color|N/A|Draw a rectangle|
|Render|Painter|drawPolygon|std::vector\<vec2> vertices, vec3/vec4/Core::Color color|N/A|Draw a polygon|
|Render|Painter|drawCircle|vec2 center, float radius, vec3/vec4/Core::Color color, int segments|N/A|Draw a solic circle (yeah it's a disc actually)|
|Render|Painter|drawRoundedRect|vec2 pos, vec2 size, float radius, vec3/vec4/Core::Color color|N/A|Draw a rounded rectangle|
|Render|Painter|drawEllipse|vec2 center, vec2 radius, vec3/vec4/Core::Color color, int segments|N/A|Draw a ellipse through x-radius and y-radius|
|Render|Painter|drawRegularPolygon|vec2 center, float radius, int sides, vec3/vec4/Core::Color color|N/A|Draw a regular polygon which has all vertices on a circle|
|Render|Painter|drawStar|vec2 center, float outerRadius, float innerRadius, int points, vec3/vec4/Core::Color color|N/A|Draw a star|
|Render|Painter|drawPie|vec2 center, float radius, float startAngle, float endAngle, vec3/vec4/Core::Color color, int segments|N/A|Draw a pie|
|Render|Painter|drawBezier|std::vector\<vec2> controlPoints, vec3/vec4/Core::Color color, int segments, float width|N/A|Draw a Bezier curve through param:controlPoints|
|Render|Painter|putImage|vec2 pos, vec2 size, Image img, vec3/vec4/Core::Color tint, vec2 UVmin, vec2UVmax|N/A|Put an image at param:pos, with tint=param:tint, tint is {1,1,1,1} in default, and UVmin and max is {0,0},{1,1}|
|Render|Painter|drawText|Font font, std::string text, vec2 pos, vec3/vec4/Core::Color color, float scale|N/A|Draw text at param:pos|
|Render|Painter|loadFont|std::string filePath, float size|Font|Load a font from file:param:filepath with size=param:size|
|Render|Painter|measureText|Font font, std::string text, float scale|vec2|Measure the size of the text under font:param:font|
|Render|Painter|createImage|std::string filepath|Image&|Load a picture from param:filepath|
|Render|Painter|resetTransform|N/A|N/A|Resets the transform of the painter|
|Render|Painter|setTransform|mat4|N/A|Sets the transform of the painter|
|Render|Painter|setProjectionMatrix|mat4|N/A|Same as above|
|Render|Painter|setViewMatrix|mat4|N/A|Same as above|
|Audio|Sound|Sound|std::string filepath|N/A|Create sound from file|
|Audio|Sound|Sound|Audio::WaveForm wave, float hz, float amplitude, long double duration|N/A|Create sound using waveform|
|Audio|Sound|Sound|Audio::NoiseColor noise, float amplitude, long double duration|N/A|Create a noise|
|Audio|Sound|soundEnded|N/A|bool|Returns true if the sound has ended|
|Audio|Sound|setAttribute|float volume, float pitch, float pan|N/A|Set the attribute of the sound|
|Audio|Engine|playSound|Sound&& sound|std::deque\<Sound>::iterator|Plays a sound|
|Audio|Engine|stopSound|std::deque\<Sound>::iterator|bool|Stops a sound|
|Game|Terrain|Terrain|std::vector\<std::vector\<std::string>>& terrain, std::unordered_map\<std::string,float> costs|N/A|Initialize a terrain|
|Game|N/A|Astar|vec2 start, vec2 goal, const Terrain& terrain, ExpandMode mode, std::function\<float(vec2,vec2)> actualCost, std::function\<float(vec2, vec2)> heuristic|std::vector\<vec2>|Standard A* algorithm|
|Game|AIBehavior|N/A|N/A|N/A|N/A|
|Game|BasicLife|N/A|N/A|N/A|N/A|
|UI|Widget|render|Render::Painter p|N/A|Renders the UI widget|
|UI|Widget|collect|N/A|std::vector\<const Widget*>|Collects all the widgets in this widget tree|
|UI|Widget|onMouseEvent|float x, float y, UI::MouseEvent e|N/A|Bubbles up the mouseEvent|
|Utils|Random|range|int min, int max|int|Get a random number from min to max|
|Utils|Random|real|float min, float max|float|Get a random **real** number from min to max|
|Utils|Timer|reset|N/A|N/A|Resets the timer|
|Utils|Timer|elapsed|N/A|double|Returns the elapsed seconds since last reset|
|Utils|Timer|reached|double seconds|bool|Returns if the elapsed seconds has reached param:seconds|
|Utils|N/A|intToRoman|int num|std::string|Convert an integer to roman number|
|Game|Scene|N/A|N/A|N/A|N/A|
|Game|SceneManager|enter|Scene s|N/A|Enter scene param:s|
|Game|SceneManager|exit|N/A|N/A|Exit top scene|
|Game|SceneManager|update|float delta|N/A|Update top scene according to param:delta|
|Render|Keyframe|addProperty|property p, float val, std::function<float(float,float,float)> interp|N/A|Adds a property to the Keyframe|
|Render|Animation|step|double len|N/A|Step the animation forward|
|Render|Animation|addKeyframe|Keyframe kf|N/A|add a Keyframe to the animation|
|Render|Animation|getMatrix|N/A|mat4|Get the matrix at this time|
|Physics|PhysicsWorld|PhysicsWorld|vec2 G|N/A|Set up a physics world with gravity=G|
|Physics|PhysicsWorld|startup|N/A|N/A|Startup the world|
|Physics|PhysicsWorld|getId|N/A|b2WorldId|Get the world id|
|Physics|PhysicsBody|PhysicsBody|b2WorldId id, vec2 pos, std::vector<vec2> polygon, float density, float friction, bool isStatic, bool unableTriangulate|N/A|Literally|
|Physics|PhysicsBody|getVertices|N/A|std::vector<vec2>|Get the vertices of the body|
|Physics|PhysicsBody|getId|N/A|b2BodyId|Get the id of the body|
|Physics|PhysicsBody|getPosition|N/A|vec2|Get the position of the body|
|Physics|PhysicsBody|getAngle|N/A|float|Get the angle of the body|
|Physics|PhysicsBody|applyForce|vec2 force, vec2 point|N/A| -- |
|Physics|PhysicsBody|applyForceToCenter|vec2 force|N/A| -- |
|Physics|PhysicsBody|applyTorque|float torque|N/A| -- |
|Physics|PhysicsBody|applyLinearImpulse|vec2 impulse, vec2 point|N/A| -- |
|Physics|PhysicsBody|applyLinearImpulseToCenter|vec2 impulse|N/A| -- |
|Physics|PhysicsBody|applyAngularImpulse|float impulse|N/A| -- |
|Physics|Joints|Please|read|the|sourcecode :D|
|Render|Particles|Same|as|above|:P|

For more details, please check the source code