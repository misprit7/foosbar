import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { EffectComposer } from 'three/examples/jsm/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/examples/jsm/postprocessing/RenderPass.js';
import { OutlinePass } from 'three/examples/jsm/postprocessing/OutlinePass.js';
import { ShaderPass } from 'three/addons/postprocessing/ShaderPass.js';
import { GammaCorrectionShader } from 'three/addons/shaders/GammaCorrectionShader.js';

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

const rodnums = [3, 5, 2, 3];
const rodnames = ['3', '5', '2', 'g'];
const limits = [2.3, 1.2, 3.52, 2.3];

const rods = new THREE.Group();
const redrods = new THREE.Group();
const bluerods = new THREE.Group();

var selection = 0;
var left_pressed = false, right_pressed = false;

const loader = new GLTFLoader();
loader.load('assets/table.glb', function(gltf) {
    const model = gltf.scene;

    // model.scale.set(5, 5, 5);
    // model.rotation.x = -Math.PI / 2;
    // model.traverse(function(object) {
    //     console.log(object);
    // });


    [['r', redrods], ['b', bluerods]].map((tup) => {
        let c = tup[0]
        let group = tup[1];
        for(let i = 0; i < rodnums.length; ++i){
            const rod = model.getObjectByName(c + rodnames[i]);
            for(let j = 0; j < rodnums[i]; ++j){
                rod.attach(model.getObjectByName(c + rodnames[i] + String(j+1)));
            }
            group.attach(rod);
        }
        rods.attach(group);
    })


    // redrods.position.z += 1;
    scene.add(model);
    scene.add(rods);
}, undefined, function(error) {
    console.error(error);
});

const ws = new WebSocket('ws://localhost:9001/position');
// console.log(ws);
ws.onmessage = (event) => {
    console.log(event);
}

function onKeyPress(event) {
    switch(event.key){
        case 'w':
        case 'ArrowUp':
            if(selection > 0) --selection;
            break;
        case 's':
        case 'ArrowDown':
            if(selection < 3) ++selection;
            break;
        case 'a':
        case 'ArrowLeft':
            left_pressed = true;
            break;
        case 'd':
        case 'ArrowRight':
            right_pressed = true;
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
        default:
    }
});

// var effect = new OutlineEffect(renderer, { defaultColor: new THREE.Color(0xff0000) });

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

function animate() {
    requestAnimationFrame(animate);

    // redrods.rotation.x += 0.03;
    // redrods.children.map((rod, i) => {
    //     if(rod.position.z > -limits[i]/2){
    //         rod.position.z -= 0.01;
    //     }
    // });

    if(selection >= 0 && redrods.children[selection]){
        const rod = redrods.children[selection];
        outlinePass.selectedObjects = [rod];
        const speed = 0.07;
        const dx = (left_pressed ?- speed : 0) + (right_pressed ? speed : 0);
        if(Math.abs(rod.position.z+dx) < limits[selection]/2){
            rod.position.z += dx;
        }
        if(Math.abs(dx)>0.001){
            const packet = {
                'rod': selection,
                'pos': rod.position.z/limits[selection]+1/2,
                'rot': rod.rotation.z,
            };
            ws.send(JSON.stringify(packet));
        }
    }

    composer.render(scene, camera);
}

animate();



const controls = new OrbitControls(camera, renderer.domElement);
controls.update();

