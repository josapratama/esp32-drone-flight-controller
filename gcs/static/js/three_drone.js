/**
 * Web 3D Drone GCS — Three.js Quadcopter & 3D Attitude Indicator
 */

class Drone3DView {
  constructor(containerId) {
    this.container = document.getElementById(containerId);
    this.scene = null;
    this.camera = null;
    this.renderer = null;
    this.droneGroup = null;
    this.rotors = [];
    this.horizonGroup = null;
    this.frontLight = null;
    
    // Smooth interpolation targets
    this.targetRoll = 0;
    this.targetPitch = 0;
    this.targetYaw = 0;
    this.currentRoll = 0;
    this.currentPitch = 0;
    this.currentYaw = 0;
    this.motorSpeeds = [0, 0, 0, 0];

    this.init();
  }

  init() {
    const width = this.container.clientWidth;
    const height = this.container.clientHeight;

    // 1. Scene
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x070b14);
    this.scene.fog = new THREE.FogExp2(0x070b14, 0.05);

    // 2. Camera
    this.camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 100);
    this.camera.position.set(0, 3.2, 5.5);
    this.camera.lookAt(0, 0, 0);

    // 3. Renderer
    this.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    this.renderer.setSize(width, height);
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    this.renderer.shadowMap.enabled = true;
    this.container.appendChild(this.renderer.domElement);

    // 4. Lights
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.6);
    this.scene.add(ambientLight);

    const dirLight = new THREE.DirectionalLight(0x00f0ff, 1.2);
    dirLight.position.set(5, 10, 7);
    this.scene.add(dirLight);

    const rearLight = new THREE.PointLight(0xff3366, 1.5, 10);
    rearLight.position.set(0, -1, -4);
    this.scene.add(rearLight);

    // 5. Ground Grid & Gimbal Ring
    this.createEnvironment();

    // 6. 3D Drone Model
    this.createDroneModel();

    // Resize listener
    window.addEventListener("resize", () => this.onWindowResize());

    // Animation Loop
    this.animate();
  }

  createEnvironment() {
    // Cyberpunk Ground Grid
    const gridHelper = new THREE.GridHelper(20, 20, 0x00f0ff, 0x1e293b);
    gridHelper.position.y = -2;
    this.scene.add(gridHelper);

    // Horizon Spherical Ring / Gimbal Cage
    this.horizonGroup = new THREE.Group();
    
    // Pitch / Roll Rings
    const ringGeo = new THREE.TorusGeometry(2.4, 0.015, 16, 64);
    const ringMat = new THREE.MeshBasicMaterial({ color: 0x38bdf8, opacity: 0.35, transparent: true });
    
    const pitchRing = new THREE.Mesh(ringGeo, ringMat);
    this.horizonGroup.add(pitchRing);

    const rollRing = new THREE.Mesh(ringGeo, ringMat);
    rollRing.rotation.y = Math.PI / 2;
    this.horizonGroup.add(rollRing);

    const yawRing = new THREE.Mesh(ringGeo, new THREE.MeshBasicMaterial({ color: 0x00f0ff, opacity: 0.2, transparent: true }));
    yawRing.rotation.x = Math.PI / 2;
    this.horizonGroup.add(yawRing);

    this.scene.add(this.horizonGroup);
  }

  createDroneModel() {
    this.droneGroup = new THREE.Group();

    // Central Carbon Fiber Body (Plate)
    const bodyGeo = new THREE.CylinderGeometry(0.55, 0.6, 0.12, 8);
    const carbonMat = new THREE.MeshStandardMaterial({
      color: 0x181e2b,
      roughness: 0.4,
      metalness: 0.8
    });
    const body = new THREE.Mesh(bodyGeo, carbonMat);
    this.droneGroup.add(body);

    // Flight Controller Dome / GPS Puck
    const fcGeo = new THREE.BoxGeometry(0.35, 0.14, 0.35);
    const fcMat = new THREE.MeshStandardMaterial({ color: 0x0f172a, roughness: 0.2, metalness: 0.9 });
    const fcBox = new THREE.Mesh(fcGeo, fcMat);
    fcBox.position.y = 0.1;
    this.droneGroup.add(fcBox);

    // Front Direction Arrow / Beacon
    const arrowGeo = new THREE.ConeGeometry(0.12, 0.25, 4);
    const arrowMat = new THREE.MeshBasicMaterial({ color: 0x00f0ff });
    const arrow = new THREE.Mesh(arrowGeo, arrowMat);
    arrow.rotation.x = -Math.PI / 2;
    arrow.position.set(0, 0.12, 0.55);
    this.droneGroup.add(arrow);

    // 4 Motor Arms (X-Configuration: M1 Front-Left, M2 Front-Right, M3 Rear-Left, M4 Rear-Right)
    const armPositions = [
      { id: "M1", x: -0.9, z: 0.9, color: 0x10b981 },   // Front-Left (Green)
      { id: "M2", x: 0.9, z: 0.9, color: 0x10b981 },    // Front-Right (Green)
      { id: "M3", x: -0.9, z: -0.9, color: 0xef4444 },  // Rear-Left (Red)
      { id: "M4", x: 0.9, z: -0.9, color: 0xef4444 },   // Rear-Right (Red)
    ];

    const armGeo = new THREE.CylinderGeometry(0.04, 0.04, 1.25, 8);
    const armMat = new THREE.MeshStandardMaterial({ color: 0x334155, roughness: 0.5, metalness: 0.7 });

    // Arm 1 (diagonal M1 to M4)
    const arm1 = new THREE.Mesh(armGeo, armMat);
    arm1.rotation.z = Math.PI / 2;
    arm1.rotation.y = Math.PI / 4;
    this.droneGroup.add(arm1);

    // Arm 2 (diagonal M2 to M3)
    const arm2 = new THREE.Mesh(armGeo, armMat);
    arm2.rotation.z = Math.PI / 2;
    arm2.rotation.y = -Math.PI / 4;
    this.droneGroup.add(arm2);

    // Brushless Motors & Propellers
    const motorGeo = new THREE.CylinderGeometry(0.12, 0.12, 0.16, 16);
    const motorMat = new THREE.MeshStandardMaterial({ color: 0x64748b, roughness: 0.3, metalness: 0.9 });
    
    // Propeller Blade Geometry
    const propGeo = new THREE.BoxGeometry(0.85, 0.015, 0.08);
    const propMat = new THREE.MeshStandardMaterial({
      color: 0x38bdf8,
      transparent: true,
      opacity: 0.85,
      roughness: 0.2
    });

    armPositions.forEach((pos) => {
      // Motor Bell
      const motor = new THREE.Mesh(motorGeo, motorMat);
      motor.position.set(pos.x, 0.1, pos.z);
      this.droneGroup.add(motor);

      // Arm LED Beacon
      const ledGeo = new THREE.SphereGeometry(0.05, 8, 8);
      const ledMat = new THREE.MeshBasicMaterial({ color: pos.color });
      const led = new THREE.Mesh(ledGeo, ledMat);
      led.position.set(pos.x, -0.05, pos.z);
      this.droneGroup.add(led);

      // Spinning Rotor Propeller
      const rotorGroup = new THREE.Group();
      const prop = new THREE.Mesh(propGeo, propMat);
      rotorGroup.add(prop);
      rotorGroup.position.set(pos.x, 0.2, pos.z);
      this.droneGroup.add(rotorGroup);
      this.rotors.push(rotorGroup);
    });

    this.scene.add(this.droneGroup);
  }

  updateAttitude(rollDeg, pitchDeg, yawDeg, m1 = 0, m2 = 0, m3 = 0, m4 = 0) {
    this.targetRoll = rollDeg;
    this.targetPitch = pitchDeg;
    this.targetYaw = yawDeg;
    this.motorSpeeds = [m1, m2, m3, m4];
  }

  onWindowResize() {
    if (!this.container) return;
    const width = this.container.clientWidth;
    const height = this.container.clientHeight;
    this.camera.aspect = width / height;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(width, height);
  }

  animate() {
    requestAnimationFrame(() => this.animate());

    // Smooth interpolation towards targets
    this.currentRoll += (this.targetRoll - this.currentRoll) * 0.2;
    this.currentPitch += (this.targetPitch - this.currentPitch) * 0.2;
    this.currentYaw += (this.targetYaw - this.currentYaw) * 0.2;

    if (this.droneGroup) {
      // Convert to radians (Aviation coordinate convention: Roll around Z/X, Pitch around X, Yaw around Y)
      this.droneGroup.rotation.order = "YXZ";
      this.droneGroup.rotation.x = THREE.MathUtils.degToRad(-this.currentPitch);
      this.droneGroup.rotation.z = THREE.MathUtils.degToRad(-this.currentRoll);
      this.droneGroup.rotation.y = THREE.MathUtils.degToRad(-this.currentYaw);

      // Spin rotors based on motor throttle PWM
      this.rotors.forEach((rotor, i) => {
        const speed = Math.max(0.05, (this.motorSpeeds[i] || 0) * 0.015);
        // Alternate CW and CCW
        const dir = (i % 2 === 0) ? 1 : -1;
        rotor.rotation.y += speed * 25.0 * dir;
      });
    }

    this.renderer.render(this.scene, this.camera);
  }
}

window.Drone3DView = Drone3DView;

