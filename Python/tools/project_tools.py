"""
Project Tools for Unreal MCP.

This module provides tools for managing project-wide settings and configuration.
"""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_project_tools(mcp: FastMCP):
    """Register project tools with the MCP server."""
    
    @mcp.tool()
    def create_input_mapping(
        ctx: Context,
        action_name: str,
        key: str,
        input_type: str = "Action"
    ) -> Dict[str, Any]:
        """
        Create an input mapping for the project.
        
        Args:
            action_name: Name of the input action
            key: Key to bind (SpaceBar, LeftMouseButton, etc.)
            input_type: Type of input mapping (Action or Axis)
            
        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "action_name": action_name,
                "key": key,
                "input_type": input_type
            }
            
            logger.info(f"Creating input mapping '{action_name}' with key '{key}'")
            response = unreal.send_command("create_input_mapping", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Input mapping creation response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating input mapping: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_input_action(
        ctx: Context,
        name: str,
        path: str = "/Game/Input/Actions",
        value_type: str = "Digital"
    ) -> Dict[str, Any]:
        """
        Create an Enhanced Input Action asset.

        Args:
            name: Name of the input action (e.g., "IA_Interact")
            path: Content path for the asset (default: /Game/Input/Actions)
            value_type: Type of input value - Digital, Axis1D, Axis2D, Axis3D (default: Digital)

        Returns:
            Dict containing success status and asset path
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "name": name,
                "path": path,
                "value_type": value_type
            }

            logger.info(f"Creating InputAction '{name}' at '{path}'")
            response = unreal.send_command("create_input_action", params)
            return response if response else {"success": False, "message": "No response"}

        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def create_input_mapping_context(
        ctx: Context,
        name: str,
        path: str = "/Game/Input"
    ) -> Dict[str, Any]:
        """
        Create an Enhanced Input Mapping Context asset.

        Args:
            name: Name of the IMC (e.g., "IMC_Player")
            path: Content path for the asset (default: /Game/Input)

        Returns:
            Dict containing success status and asset path
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"name": name, "path": path}

            logger.info(f"Creating InputMappingContext '{name}' at '{path}'")
            response = unreal.send_command("create_input_mapping_context", params)
            return response if response else {"success": False, "message": "No response"}

        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_mapping_to_context(
        ctx: Context,
        context_name: str,
        action_name: str,
        key: str
    ) -> Dict[str, Any]:
        """
        Add an InputAction mapping to an InputMappingContext.

        Args:
            context_name: Name or path of the InputMappingContext
            action_name: Name or path of the InputAction to map
            key: Key to bind (e.g., "E", "SpaceBar", "LeftMouseButton")

        Returns:
            Dict containing success status and mapping details
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "context_name": context_name,
                "action_name": action_name,
                "key": key
            }

            logger.info(f"Adding mapping: {action_name} -> {key} in {context_name}")
            response = unreal.send_command("add_mapping_to_context", params)
            return response if response else {"success": False, "message": "No response"}

        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_input_actions(
        ctx: Context,
        path: str = "/Game"
    ) -> Dict[str, Any]:
        """
        List all InputAction assets in the project.

        Args:
            path: Path filter (default: /Game)

        Returns:
            Dict containing array of InputAction assets
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_input_actions", {"path": path})
            return response if response else {"success": False, "message": "No response"}

        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_input_mapping_contexts(
        ctx: Context,
        path: str = "/Game"
    ) -> Dict[str, Any]:
        """
        List all InputMappingContext assets in the project.

        Args:
            path: Path filter (default: /Game)

        Returns:
            Dict containing array of IMC assets with their mappings
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_input_mapping_contexts", {"path": path})
            return response if response else {"success": False, "message": "No response"}

        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Project tools registered successfully") 