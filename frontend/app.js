import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { EffectComposer } from 'three/addons/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/addons/postprocessing/RenderPass.js';
import { OutlinePass } from 'three/addons/postprocessing/OutlinePass.js';
import { ShaderPass } from 'three/addons/postprocessing/ShaderPass.js';
import { GammaCorrectionShader } from 'three/addons/shaders/GammaCorrectionShader.js';
import GUI from 'lil-gui';

/******************************************************************************
 * THREE.js setup
 ******************************************************************************/

const sizes = {
    width: window.innerWidth,
    height: window.innerHeight,
};

const canvas = document.querySelector('canvas#webgl');
const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(
    75,
    sizes.width / sizes.height,
    0.1,
    1000,
);
camera.position.set(6, 10, 8); // Adjust the camera position for a good view

const renderer = new THREE.WebGLRenderer({
    antiAlias: true,
    canvas,
});
renderer.setSize(sizes.width, sizes.height);
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));

/******************************************************************************
 * Table tracking globals
 ******************************************************************************/

const rod_nums = [3, 5, 2, 3];
const rod_names = ['3', '5', '2', 'g'];
const limits = [2.3, 1.2, 3.52, 2.3];
const num_rod_t = 4;
const table_width = 6.4525,
    table_height = 11.3125;

const rods = new THREE.Group();
const red_rods = new THREE.Group();
const blue_rods = new THREE.Group();
let ball = new THREE.Group();

// Keyboard
let left_pressed = false;
let right_pressed = false;
let rot_up_pressed = false;
let rot_down_pressed = false;
let in_shot = false;
let up_dpad = false;
let down_dpad = false;

// GUI
const gui = new GUI({ width: 300, title: 'Foosbar Config' });
const guiLighting = gui.addFolder('Lighting');
guiLighting.close();
const guiControls = gui.addFolder('Controls');
guiControls.close();

const triggerKeyPress = (key, length) => {
    const event = new KeyboardEvent('keydown', {
        key,
    });
    document.dispatchEvent(event);
    setTimeout(() => {
        const event = new KeyboardEvent('keyup', {
            key,
        });
        document.dispatchEvent(event);
    }, length);
};
const keyConfig = {
    up_key: 'w',
    down_key: 's',
    left_key: 'a',
    right_key: 'd',
    rot_up_key: 'e',
    rot_down_key: 'q',
};

const keyTriggersConfig = Object.keys(keyConfig).reduce((acc, key) => {
    acc[`trigger_${key}`] = () => triggerKeyPress(keyConfig[key], 100);
    return acc;
}, {});

const globalControls = {
    selection: 0,
    ambientLightColor: '#404040',
    directionalLightColor: '#ffffff',
    ...keyConfig,
    ...keyTriggersConfig,
};

// Add keys configuration
guiControls.add(globalControls, 'up_key').name('Up key');
guiControls.add(globalControls, 'down_key').name('Down key');
guiControls.add(globalControls, 'left_key').name('Left key');
guiControls.add(globalControls, 'right_key').name('Right key');
guiControls.add(globalControls, 'rot_up_key').name('Rotate up key');
guiControls.add(globalControls, 'rot_down_key').name('Rotate down key');

// Add triggers
guiControls.add(globalControls, 'trigger_up_key').name('Trigger up key');
guiControls.add(globalControls, 'trigger_down_key').name('Trigger down key');
guiControls.add(globalControls, 'trigger_left_key').name('Trigger left key');
guiControls.add(globalControls, 'trigger_right_key').name('Trigger right key');
guiControls
    .add(globalControls, 'trigger_rot_up_key')
    .name('Trigger rotate up key');
guiControls
    .add(globalControls, 'trigger_rot_down_key')
    .name('Trigger rotate down key');

const selectionController = guiControls
    .add(globalControls, 'selection', 0, num_rod_t - 1, 1)
    .name('Selection');
let ballXController, ballZController;

/******************************************************************************
 * Add lighting
 ******************************************************************************/
const lightGroup = new THREE.Group();
scene.add(lightGroup);
const ambientLight = new THREE.AmbientLight(
    globalControls.ambientLightColor,
    8,
);
const directionalLight = new THREE.DirectionalLight(
    globalControls.directionalLightColor,
    2,
);
directionalLight.position.set(0, 5, 6);

const directionalLightHelper = new THREE.DirectionalLightHelper(
    directionalLight,
    1,
);
directionalLightHelper.visible = false;
guiLighting
    .add(ambientLight, 'intensity', 0, 10, 0.1)
    .name('Ambient Light Intensity');
guiLighting
    .addColor(ambientLight, 'color')
    .onChange((value) => {
        globalControls.ambientLightColor = value;
        ambientLight.color.set(globalControls.ambientLightColor);
    })
    .name('Ambient Light Color');
guiLighting
    .add(directionalLight, 'intensity', 0, 10, 0.1)
    .name('Directional Light Intensity');
guiLighting
    .addColor(directionalLight, 'color')
    .onChange((value) => {
        globalControls.directionalLightColor = value;
        directionalLight.color.set(globalControls.directionalLightColor);
    })
    .name('Directional Light Color');
guiLighting
    .add(directionalLightHelper, 'visible')
    .name('Show Directional Light Helper');
guiLighting
    .add(directionalLight.position, 'x')
    .min(-10)
    .max(10)
    .step(0.1)
    .name('Directional Light X');
guiLighting
    .add(directionalLight.position, 'y')
    .min(-10)
    .max(10)
    .step(0.1)
    .name('Directional Light Y');
guiLighting
    .add(directionalLight.position, 'z')
    .min(-10)
    .max(10)
    .step(0.1)
    .name('Directional Light Z');

lightGroup.add(ambientLight, directionalLight, directionalLightHelper);

/******************************************************************************
 * Add table model
 ******************************************************************************/
const loader = new GLTFLoader();
loader.load(
    '/table.glb',
    function (gltf) {
        const table = gltf.scene;

        [
            ['r', red_rods],
            ['b', blue_rods],
        ].map((tup) => {
            let c = tup[0];
            let group = tup[1];
            for (let i = 0; i < rod_nums.length; ++i) {
                const rod = table.getObjectByName(c + rod_names[i]);
                for (let j = 0; j < rod_nums[i]; ++j) {
                    rod.attach(
                        table.getObjectByName(c + rod_names[i] + String(j + 1)),
                    );
                }
                rod.offset = rod.position.z;
                group.attach(rod);
            }
            rods.attach(group);
        });

        ball = table.getObjectByName('Sphere');

        ballXController = guiControls
            .add(ball.position, 'x', -6, 6, 0.01)
            .name('Ball X');
        ballZController = guiControls
            .add(ball.position, 'z', -3.2, 3.2, 0.01)
            .name('Ball Z');

        scene.add(table, rods, ball);
    },
    undefined,
    function (error) {
        console.error(error);
    },
);

/******************************************************************************
 * Set up websocket
 ******************************************************************************/

//const ws = new WebSocket('ws://localhost:9001/position');
// const ws = new WebSocket('ws://75.157.213.247:9001/position');
const ws = new WebSocket('ws://192.168.1.77:9001/position');

// Should probably be a callback when ws connects
setTimeout(function () {
    if (ws.readyState !== WebSocket.OPEN) return;
    const packet = {
        type: 'selection',
        selection: globalControls.selection,
    };
    ws.send(JSON.stringify(packet));

    ws.onmessage = (event) => {
        const packet = JSON.parse(event.data);
        const type = packet['type'];
        if (type === 'pos') {
            for (let i = 0; i < rod_nums.length; ++i) {
                const redrod = red_rods.children[i];
                redrod.position.z =
                    redrod.offset + (packet['redpos'][i] - 1 / 2) * limits[i];
                const bluerod = blue_rods.children[i];
                bluerod.position.z =
                    bluerod.offset + (packet['bluepos'][i] - 1 / 2) * limits[i];

                red_rods.children[i].rotation.y =
                    (packet['redrot'][i] / 360) * (2 * Math.PI);
                blue_rods.children[i].rotation.y =
                    (packet['bluerot'][i] / 360) * (2 * Math.PI);
            }
            ball.position.x = packet['ballpos'][1] * table_height;
            ball.position.z = (packet['ballpos'][0] - 0.5) * table_width;

            ballXController.updateDisplay();
            ballZController.updateDisplay();
        }
    };
}, 300);

/******************************************************************************
 * Parse key presses
 ******************************************************************************/

function onKeyPress(event) {
    switch (event.key) {
        case globalControls.up_key:
        case 'ArrowUp':
            change_selection(-1);
            break;
        case globalControls.down_key:
        case 'ArrowDown':
            change_selection(1);
            break;
        case globalControls.left_key:
        case 'ArrowLeft':
            left_pressed = true;
            break;
        case 'ArrowRight':
        case globalControls.right_key:
            right_pressed = true;
            break;
        case globalControls.rot_up_key:
            rot_up_pressed = true;
            break;
        case globalControls.rot_down_key:
            rot_down_pressed = true;
            break;
        default:
    }
}
document.addEventListener('keydown', onKeyPress);

document.addEventListener('keyup', function (event) {
    switch (event.key) {
        case globalControls.left_key:
        case 'ArrowLeft':
            left_pressed = false;
            break;
        case globalControls.right_key:
        case 'ArrowRight':
            right_pressed = false;
            break;
        case globalControls.rot_up_key:
            rot_up_pressed = false;
            break;
        case globalControls.rot_down_key:
            rot_down_pressed = false;
            break;
        default:
    }
});

/******************************************************************************
 * Selection outline
 ******************************************************************************/

const composer = new EffectComposer(renderer);
const renderPass = new RenderPass(scene, camera);
composer.addPass(renderPass);

const outlinePass = new OutlinePass(
    new THREE.Vector2(sizes.width, sizes.height),
    scene,
    camera,
);
outlinePass.edgeStrength = 3;
outlinePass.edgeGlow = 1;
outlinePass.edgeThickness = 1;
outlinePass.visibleEdgeColor.set('#ffffff');
composer.addPass(outlinePass);

const gammaCorrection = new ShaderPass(GammaCorrectionShader);
composer.addPass(gammaCorrection);

/******************************************************************************
 * Helpers
 ******************************************************************************/

function change_selection(dir) {
    const { selection } = globalControls;

    if (dir === 1) {
        globalControls.selection = Math.min(selection + 1, num_rod_t - 1);
    } else if (dir === -1) {
        globalControls.selection = Math.max(selection - 1, 0);
    }
    const packet = {
        type: 'selection',
        selection,
    };
    if (ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(packet));
    }
    selectionController.updateDisplay();
}

/******************************************************************************
 * Orbit controls
 ******************************************************************************/

const controls = new OrbitControls(camera, canvas);
controls.maxPolarAngle = Math.PI / 2;
controls.enablePan = false;
controls.dampingFactor = 0.2;
controls.enableDamping = true;

/******************************************************************************
 * Main animation loop
 ******************************************************************************/

function animate() {
    controls.update();
    const { selection } = globalControls;

    if (selection >= 0 && red_rods.children[selection]) {
        const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
        const rod = red_rods.children[selection];

        outlinePass.selectedObjects = [rod];

        const lin_speed = 0.1;
        const rot_speed = 0.5;

        let dz = 0;
        let drot = 0;
        const gamepad = gamepads[0];
        if (gamepad?.id) {
            const joystickLeftX = gamepad.axes[0];
            const joystickLeftY = gamepad.axes[1]; // unused
            const joystickRightX = gamepad.axes[2]; // unused
            const joystickRightY = gamepad.axes[3];
            const leftTrigger = gamepad.buttons[6].pressed;
            const rightTrigger = gamepad.buttons[7].pressed;

            if (!in_shot && rightTrigger) {
                in_shot = true;
                drot -= Math.PI;
            }
            if (!rightTrigger) in_shot = false;

            if (gamepad.buttons[12].pressed && !up_dpad) {
                change_selection(-1);
                up_dpad = true;
            }
            if (gamepad.buttons[13].pressed && !down_dpad) {
                change_selection(1);
                down_dpad = true;
            }
            if (!gamepad.buttons[12].pressed) up_dpad = false;
            if (!gamepad.buttons[13].pressed) down_dpad = false;

            dz += (lin_speed * joystickLeftX) / (leftTrigger ? 4 : 1);
            drot += (rot_speed * joystickRightY) / (leftTrigger ? 4 : 1);
            if (Math.abs(drot) < 0.01) drot = 0;
        } else {
            dz =
                (left_pressed ? -lin_speed : 0) +
                (right_pressed ? lin_speed : 0);
            drot =
                (rot_down_pressed ? -rot_speed : 0) +
                (rot_up_pressed ? rot_speed : 0);
        }
        if (ws.readyState !== WebSocket.OPEN) {
            if (
                Math.abs(rod.position.z - rod.offset + dz) <
                limits[globalControls.selection] / 2
            ) {
                rod.position.z += dz;
            }
            rod.rotation.y += drot;
        } else if (Math.abs(dz) > 0.001 || Math.abs(drot) > 0.001) {
            const packet = {
                type: 'move',
                pos: dz / (limits[selection] * 2),
                rot: drot,
            };
            ws.send(JSON.stringify(packet));
        }
    }

    composer.render(scene, camera);
    window.requestAnimationFrame(animate);
}

animate();
