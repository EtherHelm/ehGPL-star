# ehGPL: STAR-CCM+ Viscous–Potential Flow Coupling Plugin

> [**简体中文**](docs/README.zh-CN.md) | English

A **STAR-CCM+** plugin implementing two-way viscous–potential flow coupling based on the **Ghost Potential Layer (GPL)** method.

---

## 📋 About

This plugin integrates the GPL coupling method into **STAR-CCM+**, combining a VOF-based viscous flow solver with a potential flow solver to significantly improve computational efficiency for wave simulations while maintaining accuracy.

> **Note**: This repository currently contains **only the STAR-CCM+ plugin code**, demonstrating how the plugin interacts with the backend solver. The **backend potential flow solver code is not included** but is being prepared for open-source release — stay tuned.

---

## 🚀 Quick Start

### 1. File Preparation

Download the required files from the [Releases page](https://github.com/ehw-fit/ehGPL-star/releases) and place them in the **same directory** as your STAR-CCM+ simulation files:

| File | Description |
|------|-------------|
| `libSCL.so` | Plugin shared library |
| `libducc.so` / `libducc.dll` | FFT dependency library |
| `config.ini` | Configuration file |
| `server` | GPL backend server executable |

### 2. Configure `config.ini`

Edit `config.ini` with the following parameters:

| Parameter | Description |
|-----------|-------------|
| `iniFile` | Path to the initial wave information file |
| `ipcChannel` | Inter-process communication channel |
| `watchDir` | Data exchange directory for two-way coupling |

### 3. Load the Plugin

In STAR-CCM+, load the shared library via **Tools → Load Shared Library**:

```
libSCL.so
```

---

## ⚙️ Detailed Setup

### 3.1 Define User Parameters

Navigate to **Tools → Parameters** and create the following two parameters:

| Parameter | Type | Dimension | Description |
|-----------|------|-----------|-------------|
| `dx` | Scalar | **Length** | **Grid spacing** of the potential flow background mesh |
| `nx` | Scalar | **Dimensionless** | **Number of nodes** in the potential flow background mesh |

### 3.2 Run `setup.java`

Executing `setup.java` will automatically create the following objects:

#### Region & Continuum

- **Name**: `SCL`
- **Grid size**: $n_x \times n_x$
- **Grid spacing**: `dx`

#### Automatically Created Objects

| Category | Object Name |
|----------|-------------|
| **Report** | `pushData` |
| **Monitors** | `History of User eta`, `History of User phi` |
| **Derived Part** | `wave_potential` |
| **Internal Table** | `SCL_data` (XYZ Internal Table) |

> ⚠️ **Important**: Verify that the following objects are properly associated with the **`SCL`** part:
>
> - `pushData`
> - `History of User eta`
> - `History of User phi`

### 3.3 Configure the VOF Wave Model

Navigate to **Physics → Models → VOF Wave → Waves** and create a new **User-Defined Wave**:

```
scl_wave
```

Configure the parameters as follows:

| Parameter | Setting |
|-----------|---------|
| **Free Surface Elevation** | Field function: `User etaVOF` |
| **Wave Velocity** | Field function: `User velVOF` |

### 3.4 Configure the Viscous Region

Set up the viscous region parameters as usual.

> 💡 **Tip**: Enable **VOF Wave Damping** near the boundaries of the computational domain to suppress vorticity reflection at the viscous region boundaries.

---

## 🔄 Coupling Mode Configuration

When using the **`scl_wave`** wave in the viscous region, **one-way coupling is enabled by default**.

### Enabling Two-Way Coupling

Follow these steps to enable **two-way coupling**:

#### Step 1: Create a Free Surface Iso-Surface

Create an **Iso Surface (Derived Part → Iso Surface)**:

- **Scalar field**: `Volume fraction of water`
- **Associated object**: The viscous region

This iso-surface represents the **free surface** of the viscous region.

#### Step 2: Associate the Data Table

In **`SCL_data` (XYZ Internal Table)**, associate the part with the **free surface iso-surface** created in the previous step.

#### Step 3: Toggle the Coupling Mode

| Action | Effect |
|--------|--------|
| ✅ **Enable** `SCL_data` table | Switch to **two-way coupling** |
| ❌ **Disable** `SCL_data` table | Fall back to **one-way coupling** |

---

## 📝 Notes

- The initial wave information file can be generated using the online tool: [Wave Initialization Tool](https://etherhelm.com/waveIni/)
