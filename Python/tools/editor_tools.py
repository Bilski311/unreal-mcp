"""
Editor Tools for Unreal MCP.

This module provides tools for controlling the Unreal Editor viewport and other editor functionality.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_editor_tools(mcp: FastMCP):
    """Register editor tools with the MCP server."""
    
    @mcp.tool()
    def get_actors_in_level(ctx: Context) -> List[Dict[str, Any]]:
        """Get a list of all actors in the current level."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return []
                
            response = unreal.send_command("get_actors_in_level", {})
            
            if not response:
                logger.warning("No response from Unreal Engine")
                return []
                
            # Log the complete response for debugging
            logger.info(f"Complete response from Unreal: {response}")
            
            # Check response format
            if "result" in response and "actors" in response["result"]:
                actors = response["result"]["actors"]
                logger.info(f"Found {len(actors)} actors in level")
                return actors
            elif "actors" in response:
                actors = response["actors"]
                logger.info(f"Found {len(actors)} actors in level")
                return actors
                
            logger.warning(f"Unexpected response format: {response}")
            return []
            
        except Exception as e:
            logger.error(f"Error getting actors: {e}")
            return []

    @mcp.tool()
    def find_actors_by_name(ctx: Context, pattern: str) -> List[str]:
        """Find actors by name pattern."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return []
                
            response = unreal.send_command("find_actors_by_name", {
                "pattern": pattern
            })
            
            if not response:
                return []
                
            return response.get("actors", [])
            
        except Exception as e:
            logger.error(f"Error finding actors: {e}")
            return []
    
    @mcp.tool()
    def spawn_actor(
        ctx: Context,
        name: str,
        type: str,
        location: List[float] = [0.0, 0.0, 0.0],
        rotation: List[float] = [0.0, 0.0, 0.0],
        mesh_path: str = ""
    ) -> Dict[str, Any]:
        """Create a new actor in the current level.
        
        Args:
            ctx: The MCP context
            name: The name to give the new actor (must be unique)
            type: The type of actor to create (e.g. StaticMeshActor, PointLight, DirectionalLight, SpotLight, CameraActor)
            location: The [x, y, z] world location to spawn at
            rotation: The [pitch, yaw, roll] rotation in degrees
            mesh_path: For StaticMeshActor only - path to the mesh asset to use. 
                       Common paths: /Engine/BasicShapes/Cube.Cube, /Engine/BasicShapes/Sphere.Sphere,
                       /Engine/BasicShapes/Cylinder.Cylinder, /Engine/BasicShapes/Cone.Cone, /Engine/BasicShapes/Plane.Plane
            
        Returns:
            Dict containing the created actor's properties
            
        Examples:
            # Spawn a cube at origin
            spawn_actor(ctx, "MyCube", "StaticMeshActor", [0,0,0], [0,0,0], "/Engine/BasicShapes/Cube.Cube")
            
            # Spawn a point light
            spawn_actor(ctx, "MyLight", "PointLight", [100,0,200], [0,0,0])
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            # Ensure all parameters are properly formatted
            params = {
                "name": name,
                "type": type,  # Use type as-is (e.g., "StaticMeshActor", "PointLight")
                "location": location,
                "rotation": rotation
            }
            
            # Add mesh_path if provided (for StaticMeshActor)
            if mesh_path:
                params["mesh_path"] = mesh_path
            
            # Validate location and rotation formats
            for param_name in ["location", "rotation"]:
                param_value = params[param_name]
                if not isinstance(param_value, list) or len(param_value) != 3:
                    logger.error(f"Invalid {param_name} format: {param_value}. Must be a list of 3 float values.")
                    return {"success": False, "message": f"Invalid {param_name} format. Must be a list of 3 float values."}
                # Ensure all values are float
                params[param_name] = [float(val) for val in param_value]
            
            logger.info(f"Creating actor '{name}' of type '{type}' with params: {params}")
            response = unreal.send_command("spawn_actor", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            # Log the complete response for debugging
            logger.info(f"Actor creation response: {response}")
            
            # Handle error responses correctly
            if response.get("status") == "error":
                error_message = response.get("error", "Unknown error")
                logger.error(f"Error creating actor: {error_message}")
                return {"success": False, "message": error_message}
            
            return response
            
        except Exception as e:
            error_msg = f"Error creating actor: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def delete_actor(ctx: Context, name: str) -> Dict[str, Any]:
        """Delete an actor by name."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            response = unreal.send_command("delete_actor", {
                "name": name
            })
            return response or {}
            
        except Exception as e:
            logger.error(f"Error deleting actor: {e}")
            return {}
    
    @mcp.tool()
    def set_actor_transform(
        ctx: Context,
        name: str,
        location: List[float]  = None,
        rotation: List[float]  = None,
        scale: List[float] = None
    ) -> Dict[str, Any]:
        """Set the transform of an actor."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            params = {"name": name}
            if location is not None:
                params["location"] = location
            if rotation is not None:
                params["rotation"] = rotation
            if scale is not None:
                params["scale"] = scale
                
            response = unreal.send_command("set_actor_transform", params)
            return response or {}
            
        except Exception as e:
            logger.error(f"Error setting transform: {e}")
            return {}
    
    @mcp.tool()
    def get_actor_properties(ctx: Context, name: str) -> Dict[str, Any]:
        """Get all properties of an actor."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            response = unreal.send_command("get_actor_properties", {
                "name": name
            })
            return response or {}
            
        except Exception as e:
            logger.error(f"Error getting properties: {e}")
            return {}

    @mcp.tool()
    def set_actor_property(
        ctx: Context,
        name: str,
        property_name: str,
        property_value,
    ) -> Dict[str, Any]:
        """
        Set a property on an actor.
        
        Args:
            name: Name of the actor
            property_name: Name of the property to set
            property_value: Value to set the property to
            
        Returns:
            Dict containing response from Unreal with operation status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            response = unreal.send_command("set_actor_property", {
                "name": name,
                "property_name": property_name,
                "property_value": property_value
            })
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set actor property response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting actor property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # @mcp.tool() commented out because it's buggy
    def focus_viewport(
        ctx: Context,
        target: str = None,
        location: List[float] = None,
        distance: float = 1000.0,
        orientation: List[float] = None
    ) -> Dict[str, Any]:
        """
        Focus the viewport on a specific actor or location.
        
        Args:
            target: Name of the actor to focus on (if provided, location is ignored)
            location: [X, Y, Z] coordinates to focus on (used if target is None)
            distance: Distance from the target/location
            orientation: Optional [Pitch, Yaw, Roll] for the viewport camera
            
        Returns:
            Response from Unreal Engine
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            params = {}
            if target:
                params["target"] = target
            elif location:
                params["location"] = location
            
            if distance:
                params["distance"] = distance
                
            if orientation:
                params["orientation"] = orientation
                
            response = unreal.send_command("focus_viewport", params)
            return response or {}
            
        except Exception as e:
            logger.error(f"Error focusing viewport: {e}")
            return {"status": "error", "message": str(e)}

    @mcp.tool()
    def spawn_blueprint_actor(
        ctx: Context,
        blueprint_name: str,
        actor_name: str,
        location: List[float] = [0.0, 0.0, 0.0],
        rotation: List[float] = [0.0, 0.0, 0.0]
    ) -> Dict[str, Any]:
        """Spawn an actor from a Blueprint.
        
        Args:
            ctx: The MCP context
            blueprint_name: Name of the Blueprint to spawn from
            actor_name: Name to give the spawned actor
            location: The [x, y, z] world location to spawn at
            rotation: The [pitch, yaw, roll] rotation in degrees
            
        Returns:
            Dict containing the spawned actor's properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            # Ensure all parameters are properly formatted
            params = {
                "blueprint_name": blueprint_name,
                "actor_name": actor_name,
                "location": location or [0.0, 0.0, 0.0],
                "rotation": rotation or [0.0, 0.0, 0.0]
            }
            
            # Validate location and rotation formats
            for param_name in ["location", "rotation"]:
                param_value = params[param_name]
                if not isinstance(param_value, list) or len(param_value) != 3:
                    logger.error(f"Invalid {param_name} format: {param_value}. Must be a list of 3 float values.")
                    return {"success": False, "message": f"Invalid {param_name} format. Must be a list of 3 float values."}
                # Ensure all values are float
                params[param_name] = [float(val) for val in param_value]
            
            logger.info(f"Spawning blueprint actor with params: {params}")
            response = unreal.send_command("spawn_blueprint_actor", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Spawn blueprint actor response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error spawning blueprint actor: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_actor_static_mesh(
        ctx: Context,
        name: str,
        mesh_path: str
    ) -> Dict[str, Any]:
        """Set the static mesh on a StaticMeshActor.
        
        Args:
            ctx: The MCP context
            name: Name of the StaticMeshActor to modify
            mesh_path: Path to the static mesh asset (e.g., "/Engine/BasicShapes/Cube.Cube")
            
        Common mesh paths:
            - /Engine/BasicShapes/Cube.Cube
            - /Engine/BasicShapes/Sphere.Sphere
            - /Engine/BasicShapes/Cylinder.Cylinder
            - /Engine/BasicShapes/Cone.Cone
            - /Engine/BasicShapes/Plane.Plane
            
        Returns:
            Dict containing the result of the operation
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "name": name,
                "property_name": "StaticMesh",
                "property_value": mesh_path
            }
            
            logger.info(f"Setting static mesh on actor '{name}' to '{mesh_path}'")
            response = unreal.send_command("set_actor_property", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set static mesh response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting static mesh: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_actor_components(ctx: Context, name: str) -> Dict[str, Any]:
        """Get all components attached to an actor.
        
        This is useful to discover what components an actor has and what properties
        can be modified via set_actor_component_property.
        
        Args:
            ctx: The MCP context
            name: The name of the actor to inspect
            
        Returns:
            Dict containing:
            - actor: name of the actor
            - components: list of component objects with name, class, and some properties
            - component_count: total number of components
            
        Example:
            get_actor_components(ctx, "BP_TopDownCharacter_C_UAID_...")
            -> {"actor": "...", "components": [{"name": "CharacterMovement0", "class": "CharacterMovementComponent", "MaxWalkSpeed": 600}, ...]}
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {"name": name}
            logger.info(f"Getting components for actor '{name}'")
            response = unreal.send_command("get_actor_components", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Get actor components response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error getting actor components: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_actor_component_property(
        ctx: Context,
        name: str,
        component_name: str,
        property_name: str,
        property_value: Any
    ) -> Dict[str, Any]:
        """Set a property on a component attached to an actor.

        This allows modifying properties on any component of any actor in the level.
        Use get_actor_components first to discover available components.

        Args:
            ctx: The MCP context
            name: The name of the actor
            component_name: The name or class of the component (e.g., "CharacterMovement", "CharacterMovement0", "CharacterMovementComponent")
            property_name: The property to set (e.g., "MaxWalkSpeed", "JumpZVelocity")
            property_value: The value to set (can be string, int, or float - will be converted appropriately)

        Returns:
            Dict containing success status and the set values

        Common CharacterMovementComponent properties:
            - MaxWalkSpeed (default 600) - walking speed in cm/s
            - MaxWalkSpeedCrouched (default 300) - crouched speed
            - JumpZVelocity (default 420) - jump height
            - GravityScale (default 1.0) - gravity multiplier
            - MaxAcceleration (default 2048) - how fast to reach max speed
            - BrakingDecelerationWalking (default 2048) - how fast to stop
            - GroundFriction (default 8.0) - friction when walking

        Example:
            # Make character walk faster
            set_actor_component_property(ctx, "BP_TopDownCharacter_C_...", "CharacterMovement", "MaxWalkSpeed", 1200)

            # Make character jump higher
            set_actor_component_property(ctx, "BP_TopDownCharacter_C_...", "CharacterMovement", "JumpZVelocity", 800)
        """
        from unreal_mcp_server import get_unreal_connection

        # Convert property_value to string for the C++ side
        value_str = str(property_value)

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "name": name,
                "component_name": component_name,
                "property_name": property_name,
                "property_value": value_str
            }
            logger.info(f"Setting component property: {name}.{component_name}.{property_name} = {property_value}")
            response = unreal.send_command("set_actor_component_property", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set component property response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting component property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def save_all(ctx: Context) -> Dict[str, Any]:
        """Save all modified assets and the current level in Unreal Editor.
        
        This saves:
        - The current level/map
        - Any modified assets (blueprints, materials, etc.)
        - Any dirty packages
        
        Returns:
            Dict containing:
            - success: bool indicating if save succeeded
            - saved_count: number of items saved
            - saved_items: list of saved package/level names
            - message: human-readable status message
            
        Example:
            save_all(ctx) -> {"success": True, "saved_count": 2, "saved_items": ["Level: Main", "Package: BP_MyActor"], "message": "Saved 2 item(s)"}
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info("Saving all modified assets and level...")
            response = unreal.send_command("save_all", {})
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Save all response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error saving: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    logger.info("Editor tools registered successfully")
