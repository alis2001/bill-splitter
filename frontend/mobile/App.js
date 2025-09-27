import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { Provider as PaperProvider } from 'react-native-paper';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import LoginScreen from './src/screens/LoginScreen';
import RegisterScreen from './src/screens/RegisterScreen';
import HomeScreen from './src/screens/HomeScreen';
import LoadingScreen from './src/screens/LoadingScreen';
import { AuthContext } from './src/context/AuthContext';
import { authService } from './src/services/authService';

const Stack = createStackNavigator();

export default function App() {
  const [isLoading, setIsLoading] = useState(true);
  const [userToken, setUserToken] = useState(null);
  const [user, setUser] = useState(null);

  const authContext = React.useMemo(
    () => ({
      signIn: async (email, password) => {
        try {
          const response = await authService.login(email, password);
          if (response.success) {
            await AsyncStorage.setItem('userToken', response.token);
            await AsyncStorage.setItem('user', JSON.stringify(response.user));
            setUserToken(response.token);
            setUser(response.user);
            return { success: true };
          }
          return { success: false, error: response.error };
        } catch (error) {
          return { success: false, error: 'Network error' };
        }
      },
      signUp: async (name, family_name, email, password, repeat_password) => {
        try {
          const response = await authService.register(name, family_name, email, password, repeat_password);
          if (response.success) {
            await AsyncStorage.setItem('userToken', response.token);
            await AsyncStorage.setItem('user', JSON.stringify(response.user));
            setUserToken(response.token);
            setUser(response.user);
            return { success: true };
          }
          return { success: false, error: response.error };
        } catch (error) {
          return { success: false, error: 'Network error' };
        }
      },
      signOut: async () => {
        try {
          if (userToken) {
            await authService.logout(userToken);
          }
        } catch (error) {
          console.log('Logout error:', error);
        } finally {
          await AsyncStorage.removeItem('userToken');
          await AsyncStorage.removeItem('user');
          setUserToken(null);
          setUser(null);
        }
      },
      user,
      userToken,
    }),
    [userToken, user]
  );

  useEffect(() => {
    const bootstrapAsync = async () => {
      try {
        const token = await AsyncStorage.getItem('userToken');
        const userData = await AsyncStorage.getItem('user');
        
        if (token) {
          const response = await authService.verifyToken(token);
          if (response.success) {
            setUserToken(token);
            setUser(response.user);
          } else {
            await AsyncStorage.removeItem('userToken');
            await AsyncStorage.removeItem('user');
          }
        }
      } catch (error) {
        console.log('Bootstrap error:', error);
      } finally {
        setIsLoading(false);
      }
    };

    bootstrapAsync();
  }, []);

  if (isLoading) {
    return <LoadingScreen />;
  }

  return (
    <PaperProvider>
      <SafeAreaProvider>
        <AuthContext.Provider value={authContext}>
          <NavigationContainer>
            <Stack.Navigator screenOptions={{ headerShown: false }}>
              {userToken == null ? (
                <>
                  <Stack.Screen name="Login" component={LoginScreen} />
                  <Stack.Screen name="Register" component={RegisterScreen} />
                </>
              ) : (
                <Stack.Screen name="Home" component={HomeScreen} />
              )}
            </Stack.Navigator>
          </NavigationContainer>
        </AuthContext.Provider>
      </SafeAreaProvider>
    </PaperProvider>
  );
}