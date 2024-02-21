import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { EffectComposer } from 'three/examples/jsm/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/examples/jsm/postprocessing/RenderPass.js';
import { OutlinePass } from 'three/examples/jsm/postprocessing/OutlinePass.js';
import { ShaderPass } from 'three/addons/postprocessing/ShaderPass.js';
import { GammaCorrectionShader } from 'three/addons/shaders/GammaCorrectionShader.js';

/******************************************************************************
 * THREE.js setup
 ******************************************************************************/

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
camera.position.set(6, 10, 8); // Adjust the camera position for a good view

const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

// Lighting (optional but recommended for better visuals)
const ambientLight = new THREE.AmbientLight(0x404040, 8);
scene.add(ambientLight);
const directionalLight = new THREE.DirectionalLight(0xffffff, 2);
directionalLight.position.set(0, 1, 1);
scene.add(directionalLight);

/******************************************************************************
 * Table tracking globals
 ******************************************************************************/

const rod_nums = [3, 5, 2, 3];
const rod_names = ['3', '5', '2', 'g'];
const limits = [2.3, 1.2, 3.52, 2.3];
const num_rod_t = 4;
const table_width = 6.4525, table_height = 11.3125;

const rods = new THREE.Group();
const red_rods = new THREE.Group();
const blue_rods = new THREE.Group();
let ball = new THREE.Group();

// Keyboard
var selection = 0;
var left_pressed = false, right_pressed = false;
var rot_up_pressed = false, rot_down_pressed = false;

// Gamepad
var dpad_y_last = 0;

/******************************************************************************
 * Add table model
 ******************************************************************************/
const loader = new GLTFLoader();
loader.load('assets/table.glb', function(gltf) {
    const table = gltf.scene;

    [['r', red_rods], ['b', blue_rods]].map((tup) => {
        let c = tup[0]
        let group = tup[1];
        for(let i = 0; i < rod_nums.length; ++i){
            const rod = table.getObjectByName(c + rod_names[i]);
            for(let j = 0; j < rod_nums[i]; ++j){
                rod.attach(table.getObjectByName(c + rod_names[i] + String(j+1)));
            }
            rod.offset = rod.position.z;
            group.attach(rod);
        }
        rods.attach(group);
    })
    // console.log(table.children);

    ball = table.getObjectByName("Sphere");

    // var axesHelper = new THREE.AxesHelper(5);
    // table.add(axesHelper);
    scene.add(table);
    // var axesHelper = new THREE.AxesHelper(5);
    // redrods.children[0].add(axesHelper);
    scene.add(rods);
    scene.add(ball);
}, undefined, function(error) {
    console.error(error);
});

/******************************************************************************
 * Set up websocket
 ******************************************************************************/

const ws = new WebSocket('ws://localhost:9001/position');
// const ws = new WebSocket('ws://75.157.213.247:9001/position');
// const ws = new WebSocket('ws://192.168.1.77:9001/position');

// Should probably be a callback when ws connects
setTimeout(function() {
    const packet = {
        'type': 'selection',
        'selection': selection,
    };
    ws.send(JSON.stringify(packet));

    ws.onmessage = (event) => {
        const packet = JSON.parse(event.data);
        const type = packet['type']
        if(type == 'pos'){
            for(let i = 0; i < rod_nums.length; ++i){
                const redrod = red_rods.children[i];
                redrod.position.z = redrod.offset + (packet['redpos'][i]-1/2)*limits[i];
                const bluerod = blue_rods.children[i];
                bluerod.position.z = bluerod.offset + (packet['bluepos'][i]-1/2)*limits[i];

                red_rods.children[i].rotation.y = (packet['redrot'][i] / 360 * (2*Math.PI));
                blue_rods.children[i].rotation.y = (packet['bluerot'][i] / 360 * (2*Math.PI));
            }
            ball.position.x = (packet['ballpos'][1]) * table_height;
            ball.position.z = (packet['ballpos'][0] - 0.5) * table_width;
        }
    }
}, 300);

/******************************************************************************
 * Parse key presses
 ******************************************************************************/

function onKeyPress(event) {
    switch(event.key){
        case 'w':
        case 'ArrowUp':
            change_selection(-1);
            break;
        case 's':
        case 'ArrowDown':
            change_selection(1);
            break;
        case 'a':
        case 'ArrowLeft':
            left_pressed = true;
            break;
        case 'd':
        case 'ArrowRight':
            right_pressed = true;
            break;
        case 'e':
            rot_up_pressed = true;
            break;
        case 'q':
            rot_down_pressed = true;
            break;
        default:
    }
}
document.addEventListener('keydown', onKeyPress);

document.addEventListener('keyup', function(event) {
    switch(event.key){
        case 'a':
        case 'ArrowLeft':
            left_pressed = false;
            break;
        case 'd':
        case 'ArrowRight':
            right_pressed = false;
            break;
        case 'e':
            rot_up_pressed = false;
            break;
        case 'q':
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

const outlinePass = new OutlinePass(new THREE.Vector2(window.innerWidth, window.innerHeight), scene, camera);
outlinePass.edgeStrength = 3;
outlinePass.edgeGlow = 1;
outlinePass.edgeThickness = 1;
outlinePass.visibleEdgeColor.set('#ffffff');
composer.addPass(outlinePass);

const gammaCorrection = new ShaderPass( GammaCorrectionShader );
composer.addPass( gammaCorrection );

/******************************************************************************
 * Helpers
 ******************************************************************************/

function change_selection(dir){
    if(dir == 1){
        selection = Math.min(selection+1, num_rod_t-1);
    } else if (dir == -1){
        selection = Math.max(selection-1, 0);
    }
    const packet = {
        'type': 'selection',
        'selection': selection,
    };
    ws.send(JSON.stringify(packet));
}

/******************************************************************************
 * Orbit controls
 ******************************************************************************/

const controls = new OrbitControls(camera, renderer.domElement);
controls.update();


/******************************************************************************
 * Main animation loop
 ******************************************************************************/


function animate() {
    requestAnimationFrame(animate);

    if(selection >= 0 && red_rods.children[selection]){
        const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
        const rod = red_rods.children[selection];

        outlinePass.selectedObjects = [rod];

        const lin_speed = 0.10;
        const rot_speed = 0.10;

        let dz = 0, drot = 0;
        if(gamepads.length > 0){
            const gamepad = gamepads[0];
            const joystickLeftX = gamepad.axes[0];
            const joystickLeftY = gamepad.axes[1];
            const joystickRightX = gamepad.axes[2];
            const joystickRightY = gamepad.axes[3];
            const leftTrigger = gamepad.buttons[7].pressed;

            // console.log(gamepad.axes[5]);
            if(gamepad.axes[5] != dpad_y_last){
                dpad_y_last = gamepad.axes[5];
                if(dpad_y_last != 0){
                    change_selection(dpad_y_last > 0.5 ? 1 : -1);
                }
            }

            dz = lin_speed * joystickLeftX / (leftTrigger ? 4 : 1);
            drot = rot_speed * joystickRightY / (leftTrigger ? 4 : 1);
        } else {
            dz = (left_pressed ?- lin_speed : 0) + (right_pressed ? lin_speed : 0);
            drot = (rot_down_pressed ?- rot_speed : 0) + (rot_up_pressed ? rot_speed : 0);
        }
        if(ws.readyState != WebSocket.OPEN){
            if(Math.abs(rod.position.z-rod.offset+dz) < limits[selection]/2){
                rod.position.z += dz;
            }
            rod.rotation.y += drot;
        } else if(Math.abs(dz)>0.001 || Math.abs(drot)>0.001){
            const packet = {
                'type': 'move',
                'pos': dz/(limits[selection]*2),
                'rot': drot,
            };
            // console.log(drot)
            ws.send(JSON.stringify(packet));
        }
    }

    composer.render(scene, camera);
}

animate();

