import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
camera.position.set(0, 5, 10); // Adjust the camera position for a good view

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
                // console.log(c + rodnames[i] + (j+1));
                // console.log(rod);
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
console.log(ws);
ws.onmessage = (event) => {
    console.log(event);
}

function animate() {
    requestAnimationFrame(animate);

    // redrods.rotation.x += 0.03;
    redrods.children.map((rod, i) => {
        if(rod.position.z > -limits[i]/2){
            rod.position.z -= 0.01;
            // console.log(rod.position.z);
        }
    });

    renderer.render(scene, camera);
}

animate();



const controls = new OrbitControls(camera, renderer.domElement);
controls.update();

