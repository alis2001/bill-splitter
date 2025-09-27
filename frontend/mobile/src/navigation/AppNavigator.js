import React from 'react';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createStackNavigator } from '@react-navigation/stack';
import { Ionicons } from '@expo/vector-icons';

// Import screens
import EventsListScreen from '../screens/events/EventsListScreen';
import EventDetailsScreen from '../screens/events/EventDetailsScreen';
import AddExpenseScreen from '../screens/expenses/AddExpenseScreen';
import BalanceScreen from '../screens/balance/BalanceScreen';
import ProfileScreen from '../screens/profile/ProfileScreen';
import CreateEventScreen from '../screens/events/CreateEventScreen';
import SettleScreen from '../screens/settlements/SettleScreen';

const Tab = createBottomTabNavigator();
const Stack = createStackNavigator();

// Events Stack
function EventsStack() {
  return (
    <Stack.Navigator screenOptions={{ headerShown: false }}>
      <Stack.Screen name="EventsList" component={EventsListScreen} />
      <Stack.Screen name="EventDetails" component={EventDetailsScreen} />
      <Stack.Screen name="AddExpense" component={AddExpenseScreen} />
      <Stack.Screen name="CreateEvent" component={CreateEventScreen} />
      <Stack.Screen name="Settle" component={SettleScreen} />
    </Stack.Navigator>
  );
}

// Main Tab Navigator
export default function AppNavigator() {
  return (
    <Tab.Navigator
      screenOptions={({ route }) => ({
        tabBarIcon: ({ focused, color, size }) => {
          let iconName;

          if (route.name === 'Events') {
            iconName = focused ? 'list' : 'list-outline';
          } else if (route.name === 'Balance') {
            iconName = focused ? 'wallet' : 'wallet-outline';
          } else if (route.name === 'Profile') {
            iconName = focused ? 'person' : 'person-outline';
          }

          return <Ionicons name={iconName} size={size} color={color} />;
        },
        tabBarActiveTintColor: '#1A365D',
        tabBarInactiveTintColor: '#A0AEC0',
        headerShown: false,
        tabBarStyle: {
          backgroundColor: '#FFFFFF',
          borderTopWidth: 1,
          borderTopColor: '#E2E8F0',
          paddingBottom: 8,
          paddingTop: 8,
          height: 60,
        },
        tabBarLabelStyle: {
          fontSize: 12,
          fontWeight: '500',
        },
      })}
    >
      <Tab.Screen name="Events" component={EventsStack} />
      <Tab.Screen name="Balance" component={BalanceScreen} />
      <Tab.Screen name="Profile" component={ProfileScreen} />
    </Tab.Navigator>
  );
}