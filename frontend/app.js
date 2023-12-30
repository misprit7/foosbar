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
 * Rod tracking globals
 ******************************************************************************/

const rodnums = [3, 5, 2, 3];
const rodnames = ['3', '5', '2', 'g'];
const limits = [2.3, 1.2, 3.52, 2.3];

const rods = new THREE.Group();
const redrods = new THREE.Group();
const bluerods = new THREE.Group();

var selection = 0;
var left_pressed = false, right_pressed = false;
var rot_up_pressed = false, rot_down_pressed = false;

/******************************************************************************
 * Add table model
 ******************************************************************************/
const loader = new GLTFLoader();
loader.load('assets/table.glb', function(gltf) {
    const table = gltf.scene;

    [['r', redrods], ['b', bluerods]].map((tup) => {
        let c = tup[0]
        let group = tup[1];
        for(let i = 0; i < rodnums.length; ++i){
            const rod = table.getObjectByName(c + rodnames[i]);
            for(let j = 0; j < rodnums[i]; ++j){
                rod.attach(table.getObjectByName(c + rodnames[i] + String(j+1)));
            }
            rod.offset = rod.position.z;
            group.attach(rod);
        }
        rods.attach(group);
    })

    // var axesHelper = new THREE.AxesHelper(5);
    // table.add(axesHelper);
    scene.add(table);
    // var axesHelper = new THREE.AxesHelper(5);
    // redrods.children[0].add(axesHelper);
    scene.add(rods);
}, undefined, function(error) {
    console.error(error);
});

/******************************************************************************
 * Set up websocket
 ******************************************************************************/

const ws = new WebSocket('ws://localhost:9001/position');

// Should probably be a callback when ws connects
setTimeout(function() {
    const packet = {
        'type': 'selection',
        'selection': selection,
    };
    ws.send(JSON.stringify(packet));
    console.log(redrods.children[0]);

    ws.onmessage = (event) => {
        // console.log(event);
        const packet = JSON.parse(event.data);
        for(let i = 0; i < rodnums.length; ++i){
            const redrod = redrods.children[i];
            redrod.position.z = redrod.offset + (packet['redpos'][i]-1/2)*limits[i];
            const bluerod = bluerods.children[i];
            bluerod.position.z = bluerod.offset + (packet['bluepos'][i]-1/2)*limits[i];
            redrods.children[i].rotation.y = (packet['redrot'][i]);
            bluerods.children[i].rotation.y = (packet['bluerot'][i]);
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
            if(selection > 0){
                --selection;
                const packet = {
                    'type': 'selection',
                    'selection': selection,
                };
                ws.send(JSON.stringify(packet));
            }
            break;
        case 's':
        case 'ArrowDown':
            if(selection < 3){
                ++selection;
                const packet = {
                    'type': 'selection',
                    'selection': selection,
                };
                ws.send(JSON.stringify(packet));
            }
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
 * Orbit controls
 ******************************************************************************/

const controls = new OrbitControls(camera, renderer.domElement);
controls.update();

/******************************************************************************
 * Main animation loop
 ******************************************************************************/


function animate() {
    requestAnimationFrame(animate);

    if(selection >= 0 && redrods.children[selection]){
        const rod = redrods.children[selection];
        outlinePass.selectedObjects = [rod];
        const lin_speed = 0.07;
        const rot_speed = 0.01;
        const dz = (left_pressed ?- lin_speed : 0) + (right_pressed ? lin_speed : 0);
        const drot = (rot_down_pressed ?- rot_speed : 0) + (rot_up_pressed ? rot_speed : 0);
        if(ws.readyState != WebSocket.OPEN){
            if(Math.abs(rod.position.z+dz) < limits[selection]/2){
                rod.position.z += dz;
            }
            rod.rotation.y += drot;
        }
        if(Math.abs(dz)>0.01){
            const packet = {
                'type': 'move',
                'pos': dz/(limits[selection]*2),
                'rot': 0,
            };
            ws.send(JSON.stringify(packet));
        }
    }

    composer.render(scene, camera);
}

animate();

